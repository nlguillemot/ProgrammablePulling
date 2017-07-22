#include "buddha.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "wavefront.h"

#include <iostream>
#include <fstream>

// Keep this in sync with struct CachedVertex
#define VERTEX_CACHE_VERTEX_SIZE_IN_DWORDS 12
// Keep this in sync with CacheEntry
#define VERTEX_CACHE_ENTRY_SIZE_IN_DWORDS 4

namespace buddha {
    
struct Transform {
	glm::mat4 ModelViewMatrix;		// modelview matrix of the transformation
	glm::mat4 ProjectionMatrix;		// projection matrix of the transformation
	glm::mat4 MVPMatrix;			// modelview-projection matrix
};

struct Camera {
	glm::vec3 position;				// camera position
	glm::vec3 rotation;				// camera rotation
};

struct DrawArraysCmd {
    GLuint count = 0;
    GLuint instanceCount = 1;
    GLuint first = 0;
    GLuint baseInstance = 0;
};

struct DrawElementsCmd {
    GLuint count = 0;
    GLuint instanceCount = 1;
    GLuint firstIndex = 0;
    GLuint baseVertex = 0;
    GLuint baseInstance = 0;
};

enum DrawCmdType
{
    DRAWCMD_UNKNOWN,
    DRAWCMD_DRAWARRAYS,
    DRAWCMD_DRAWELEMENTS
};

struct DrawCommand
{
    GLuint vertexArray = 0;
    GLenum primType = GL_TRIANGLES;

    GLint patchVertices = 0;
    GLfloat patchDefaultOuterLevel[4]{ 1,1,1,1 };
    GLfloat patchDefaultInnerLevel[2]{ 1,1 };

    // specifies whether this is an indexed draw command
    DrawCmdType drawType = DRAWCMD_UNKNOWN;
    DrawElementsCmd drawElements;
    DrawArraysCmd drawArrays;
};

class BuddhaDemo : public IBuddhaDemo
{
    Camera camera;                          // camera data

    Transform transform;                    // transformation data
    GLuint transformUB;                     // uniform buffer for the transformation

    struct VertexProg {
        GLuint prog;
        std::string name;
    };
    GLuint fragmentProg;                    // common fragment shader program
    VertexProg vertexProg[NUMBER_OF_MODES_INCLUDING_DISABLED_ONES];     // vertex shader programs for the three vertex pulling modes
    GLuint progPipeline[NUMBER_OF_MODES_INCLUDING_DISABLED_ONES];   // program pipelines for the three vertex pulling modes

    struct PerModel
    {
        // index buffer for the mesh
        GLuint indexBuffer;

        // separate index/vertex buffers
        GLuint positionIndexBuffer;
        GLuint normalIndexBuffer;
        GLuint uniquePositionBufferXYZW;
        GLuint uniqueNormalBufferXYZW;

        int numUniqueVerts;

        GLuint assemblyIndexBuffer;
        GLuint assemblyVertexArray;

        // vertex buffers for various settings
        GLuint interleavedBuffer;
        GLuint positionBuffer;
        GLuint normalBuffer;
        GLuint positionBufferXYZW;
        GLuint normalBufferXYZW;

        GLuint positionXBuffer;
        GLuint positionYBuffer;
        GLuint positionZBuffer;
        GLuint normalXBuffer;
        GLuint normalYBuffer;
        GLuint normalZBuffer;

        GLuint nullVertexArray;
        GLuint vertexArrayIndexBufferOnly;
        GLuint vertexArrayInterleaved;
        GLuint vertexArrayAoS;
        GLuint vertexArrayAoSXYZW;
        GLuint vertexArraySoA;

        // texture handles to the vertex buffers
        GLuint indexTexBufferR32I;
        GLuint positionTexBufferR32F;
        GLuint normalTexBufferR32F;
        GLuint positionTexBufferRGB32F;
        GLuint normalTexBufferRGB32F;
        GLuint positionTexBufferRGBA32F;
        GLuint normalTexBufferRGBA32F;

        GLuint positionXTexBufferR32F;
        GLuint positionYTexBufferR32F;
        GLuint positionZTexBufferR32F;
        GLuint normalXTexBufferR32F;
        GLuint normalYTexBufferR32F;
        GLuint normalZTexBufferR32F;

        GLuint vertexCacheBuffer;

        DrawCommand drawCmd[NUMBER_OF_MODES_INCLUDING_DISABLED_ONES];   // draw command for the three vertex pulling modes

        void load(const char* path);
    };

    std::vector<PerModel> models;

    GLuint timeElapsedQuery;                // query object for the time taken to render the scene

    float cameraRotationFactor;             // camera rotation factor between [0,2*PI)

    GLuint vertexCacheCounterBuffer;
    GLuint vertexCacheBucketsBuffer;
    GLuint vertexCacheBucketLocksBuffer;

    GLuint vertexCacheMissCounterBuffer;
    GLuint vertexCacheMissCounterReadbackBuffer;

    SoftVertexCacheConfig softVertexCacheConfig;

    int lastFrameNumVertexCacheMisses;
    int lastFrameMeshID;

    void loadShaders();

    VertexProg loadShaderProgramFromFile(const char* filename, const char* preamble, GLenum shaderType);
    GLuint createProgramPipeline(GLuint vertexShader, GLuint tessControlShader, GLuint tessEvaluationShader, GLuint geometryShader, GLuint fragmentShader);
    
    GLuint createProgramPipeline(const VertexProg& vertexShader, GLuint tessControlShader, GLuint tessEvaluationShader, GLuint geometryShader, GLuint fragmentShader)
    {
        return createProgramPipeline(vertexShader.prog, tessControlShader, tessEvaluationShader, geometryShader, fragmentShader);
    }

public:
    BuddhaDemo();

    int addMesh(const char* path) override;

    void renderScene(int meshID, const glm::mat4& modelMatrix, int screenWidth, int screenHeight, float dtsec, VertexPullingMode mode, uint64_t* elapsedNanoseconds) override;

    std::string GetModeName(int mode) override
    {
        return vertexProg[mode].name;
    }

    SoftVertexCacheConfig GetSoftVertexCacheConfig() const override;

    void SetSoftVertexCacheConfig(const SoftVertexCacheConfig& config) override;

    void GetSoftVertexCacheStats(int* pNumCacheMisses, int* pTotalNumVerts) const override
    {
        if (pNumCacheMisses)
            *pNumCacheMisses = lastFrameNumVertexCacheMisses;
        if (pTotalNumVerts)
            *pTotalNumVerts = models[lastFrameMeshID].numUniqueVerts;
    }

    void* operator new(size_t sz)
    {
        void* m = malloc(sz);
        memset(m, 0, sz);
        return m;
    }

    void operator delete(void* mem)
    {
        free(mem);
    }
};

std::shared_ptr<IBuddhaDemo> IBuddhaDemo::Create()
{
    return std::make_shared<BuddhaDemo>();
}

void BuddhaDemo::PerModel::load(const char* path)
{
    demo::WaveFrontObj buddhaObj(path);

    // Index buffer
    {
        GLsizei bufferSize = (GLsizei)(buddhaObj.Indices.size() * sizeof(GLuint));
        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, indexBuffer);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, buddhaObj.Indices.data(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // position index buffer
    {
        GLsizei bufferSize = (GLsizei)(buddhaObj.PositionIndices.size() * sizeof(GLuint));
        glGenBuffers(1, &positionIndexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, positionIndexBuffer);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, buddhaObj.PositionIndices.data(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // normal index buffer
    {
        GLsizei bufferSize = (GLsizei)(buddhaObj.NormalIndices.size() * sizeof(GLuint));
        glGenBuffers(1, &normalIndexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, normalIndexBuffer);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, buddhaObj.NormalIndices.data(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    numUniqueVerts = int(buddhaObj.PositionIndices.size());

    // unique position buffer
    {
        std::unique_ptr<glm::vec4[]> positions(new glm::vec4[buddhaObj.UniquePositions.size()]);
        for (size_t i = 0; i < buddhaObj.UniquePositions.size(); i++)
        {
            positions[i][0] = buddhaObj.UniquePositions[i][0];
            positions[i][1] = buddhaObj.UniquePositions[i][1];
            positions[i][2] = buddhaObj.UniquePositions[i][2];
            positions[i][3] = 1.0f;
        }

        GLsizei bufferSize = (GLsizei)(buddhaObj.UniquePositions.size() * sizeof(glm::vec4));
        glGenBuffers(1, &uniquePositionBufferXYZW);
        glBindBuffer(GL_ARRAY_BUFFER, uniquePositionBufferXYZW);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, positions.get(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // unique normal buffer
    {
        std::unique_ptr<glm::vec4[]> normals(new glm::vec4[buddhaObj.UniqueNormals.size()]);
        for (size_t i = 0; i < buddhaObj.UniqueNormals.size(); i++)
        {
            normals[i][0] = buddhaObj.UniqueNormals[i][0];
            normals[i][1] = buddhaObj.UniqueNormals[i][1];
            normals[i][2] = buddhaObj.UniqueNormals[i][2];
            normals[i][3] = 0.0f;
        }

        GLsizei bufferSize = (GLsizei)(buddhaObj.UniqueNormals.size() * sizeof(glm::vec4));
        glGenBuffers(1, &uniqueNormalBufferXYZW);
        glBindBuffer(GL_ARRAY_BUFFER, uniqueNormalBufferXYZW);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, normals.get(), 0);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // "assembly" index buffer
    {
        std::unique_ptr<GLuint[]> assemblyIndices(new GLuint[buddhaObj.PositionIndices.size() + buddhaObj.NormalIndices.size()]);
        for (size_t i = 0; i < buddhaObj.PositionIndices.size(); i++)
        {
            assemblyIndices[i * 2 + 0] = buddhaObj.PositionIndices[i];
            assemblyIndices[i * 2 + 1] = buddhaObj.NormalIndices[i] | 0x80000000;
        }

        GLsizei bufferSize = (GLsizei)((buddhaObj.PositionIndices.size() + buddhaObj.NormalIndices.size()) * sizeof(GLuint));
        glGenBuffers(1, &assemblyIndexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, assemblyIndexBuffer);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, assemblyIndices.get(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    struct Interleaved
    {
        glm::vec3 Position;
        glm::vec3 Normal;
    };
    static_assert(sizeof(Interleaved) == sizeof(float) * 6, "assume tightly packed");

    // AoS interleaved buffer
    {
        std::unique_ptr<Interleaved[]> interleaved(new Interleaved[buddhaObj.Positions.size()]);
        for (size_t i = 0; i < buddhaObj.Positions.size(); i++)
        {
            interleaved[i].Position[0] = buddhaObj.Positions[i][0];
            interleaved[i].Position[1] = buddhaObj.Positions[i][1];
            interleaved[i].Position[2] = buddhaObj.Positions[i][2];
            interleaved[i].Normal[0] = buddhaObj.Normals[i][0];
            interleaved[i].Normal[1] = buddhaObj.Normals[i][1];
            interleaved[i].Normal[2] = buddhaObj.Normals[i][2];
        }

        GLsizei bufferSize = (GLsizei)(
            buddhaObj.Positions.size() * sizeof(glm::vec3) +
            buddhaObj.Normals.size() * sizeof(glm::vec3));

        glGenBuffers(1, &interleavedBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, interleavedBuffer);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, interleaved.get(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

	// AoS position buffer
    {
        GLsizei bufferSize = (GLsizei)(buddhaObj.Positions.size() * sizeof(glm::vec3));
        glGenBuffers(1, &positionBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, buddhaObj.Positions.data(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // AoS position buffer XYZW
    {
        std::unique_ptr<glm::vec4[]> positions(new glm::vec4[buddhaObj.Positions.size()]);
        for (size_t i = 0; i < buddhaObj.Positions.size(); i++)
        {
            positions[i][0] = buddhaObj.Positions[i][0];
            positions[i][1] = buddhaObj.Positions[i][1];
            positions[i][2] = buddhaObj.Positions[i][2];
            positions[i][3] = 1.0f;
        }

        GLsizei bufferSize = (GLsizei)(buddhaObj.Positions.size() * sizeof(glm::vec4));
        glGenBuffers(1, &positionBufferXYZW);
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferXYZW);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, positions.get(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // AoS normal buffer
    {
        GLsizei bufferSize = (GLsizei)(buddhaObj.Normals.size() * sizeof(glm::vec3));
        glGenBuffers(1, &normalBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, buddhaObj.Normals.data(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // AoS normal buffer XYZW
    {
        std::unique_ptr<glm::vec4[]> normals(new glm::vec4[buddhaObj.Normals.size()]);
        for (size_t i = 0; i < buddhaObj.Normals.size(); i++)
        {
            normals[i][0] = buddhaObj.Normals[i][0];
            normals[i][1] = buddhaObj.Normals[i][1];
            normals[i][2] = buddhaObj.Normals[i][2];
            normals[i][3] = 0.0f;
        }

        GLsizei bufferSize = (GLsizei)(buddhaObj.Normals.size() * sizeof(glm::vec4));
        glGenBuffers(1, &normalBufferXYZW);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferXYZW);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, normals.get(), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // SoA buffers
    for (size_t soaIdx = 0; soaIdx < 6; soaIdx++)
    {
        GLuint* pVertexBuffer =
            soaIdx == 0 ? &positionXBuffer          :
            soaIdx == 1 ? &positionYBuffer          :
            soaIdx == 2 ? &positionZBuffer          :
            soaIdx == 3 ? &normalXBuffer            :
            soaIdx == 4 ? &normalYBuffer            :
            soaIdx == 5 ? &normalZBuffer            :
            NULL;

        if (soaIdx >= 0 && soaIdx <= 5)
        {
            std::unique_ptr<float[]> buf(new float[buddhaObj.Positions.size()]);
            for (size_t i = 0; i < buddhaObj.Positions.size(); i++)
            {
                buf[i] =
                    soaIdx == 0 ? buddhaObj.Positions[i].x :
                    soaIdx == 1 ? buddhaObj.Positions[i].y :
                    soaIdx == 2 ? buddhaObj.Positions[i].z :
                    soaIdx == 3 ? buddhaObj.Normals[i].x :
                    soaIdx == 4 ? buddhaObj.Normals[i].y :
                    soaIdx == 5 ? buddhaObj.Normals[i].z :
                    0.0f;
            }

            GLsizei bufferSize = (GLsizei)(buddhaObj.Positions.size() * sizeof(float));
            glGenBuffers(1, pVertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
            glBufferStorage(GL_ARRAY_BUFFER, bufferSize, buf.get(), 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

	// setup vertex arrays

    glGenVertexArrays(1, &nullVertexArray);
    glBindVertexArray(nullVertexArray);
    // binding it just creates it...
    glBindVertexArray(0);

    glGenVertexArrays(1, &vertexArrayIndexBufferOnly);
    glBindVertexArray(vertexArrayIndexBufferOnly);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBindVertexArray(0);

    glGenVertexArrays(1, &vertexArrayInterleaved);
    glBindVertexArray(vertexArrayInterleaved);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, interleavedBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Interleaved), (GLvoid*)offsetof(Interleaved, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Interleaved), (GLvoid*)offsetof(Interleaved, Normal));
    glBindVertexArray(0);

    glGenVertexArrays(1, &vertexArrayAoS);
    glBindVertexArray(vertexArrayAoS);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &vertexArrayAoSXYZW);
    glBindVertexArray(vertexArrayAoSXYZW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBufferXYZW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
    glBindBuffer(GL_ARRAY_BUFFER, normalBufferXYZW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &vertexArraySoA);
    glBindVertexArray(vertexArraySoA);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionXBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, positionYBuffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, positionZBuffer);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, normalXBuffer);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, normalYBuffer);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, normalZBuffer);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &assemblyVertexArray);
    glBindVertexArray(assemblyVertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assemblyIndexBuffer);
    glBindVertexArray(0);

    drawCmd[FIXED_FUNCTION_AOS_MODE].vertexArray = vertexArrayAoS;
    drawCmd[FIXED_FUNCTION_AOS_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FIXED_FUNCTION_AOS_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[FIXED_FUNCTION_AOS_XYZW_MODE].vertexArray = vertexArrayAoSXYZW;
    drawCmd[FIXED_FUNCTION_AOS_XYZW_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FIXED_FUNCTION_AOS_XYZW_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[FIXED_FUNCTION_SOA_MODE].vertexArray = vertexArraySoA;
    drawCmd[FIXED_FUNCTION_SOA_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FIXED_FUNCTION_SOA_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[FIXED_FUNCTION_INTERLEAVED_MODE].vertexArray = vertexArrayInterleaved;
    drawCmd[FIXED_FUNCTION_INTERLEAVED_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FIXED_FUNCTION_INTERLEAVED_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[FETCHER_AOS_1RGBAFETCH_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_AOS_1RGBAFETCH_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FETCHER_AOS_1RGBAFETCH_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();
    drawCmd[FETCHER_AOS_1RGBFETCH_MODE] = drawCmd[FETCHER_AOS_1RGBAFETCH_MODE];
    drawCmd[FETCHER_AOS_3FETCH_MODE] = drawCmd[FETCHER_AOS_1RGBAFETCH_MODE];

    drawCmd[FETCHER_SOA_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_SOA_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FETCHER_SOA_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[FETCHER_IMAGE_AOS_1FETCH_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_IMAGE_AOS_1FETCH_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FETCHER_IMAGE_AOS_1FETCH_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();
    drawCmd[FETCHER_IMAGE_AOS_3FETCH_MODE] = drawCmd[FETCHER_IMAGE_AOS_1FETCH_MODE];

    drawCmd[FETCHER_IMAGE_SOA_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_IMAGE_SOA_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FETCHER_IMAGE_SOA_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[FETCHER_SSBO_AOS_1FETCH_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_SSBO_AOS_1FETCH_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FETCHER_SSBO_AOS_1FETCH_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();
    drawCmd[FETCHER_SSBO_AOS_3FETCH_MODE] = drawCmd[FETCHER_SSBO_AOS_1FETCH_MODE];

    drawCmd[FETCHER_SSBO_SOA_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_SSBO_SOA_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[FETCHER_SSBO_SOA_MODE].drawElements.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_AOS_1RGBAFETCH_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_AOS_1RGBAFETCH_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_AOS_1RGBAFETCH_MODE].drawArrays.count = (GLuint)buddhaObj.Indices.size();
    drawCmd[PULLER_AOS_1RGBFETCH_MODE] = drawCmd[PULLER_AOS_1RGBAFETCH_MODE];
    drawCmd[PULLER_AOS_3FETCH_MODE] = drawCmd[PULLER_AOS_1RGBAFETCH_MODE];

    drawCmd[PULLER_SOA_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_SOA_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_SOA_MODE].drawArrays.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_IMAGE_AOS_1FETCH_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_IMAGE_AOS_1FETCH_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_IMAGE_AOS_1FETCH_MODE].drawArrays.count = (GLuint)buddhaObj.Indices.size();
    drawCmd[PULLER_IMAGE_AOS_3FETCH_MODE] = drawCmd[PULLER_IMAGE_AOS_1FETCH_MODE];

    drawCmd[PULLER_IMAGE_SOA_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_IMAGE_SOA_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_IMAGE_SOA_MODE].drawArrays.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_SSBO_AOS_1FETCH_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_SSBO_AOS_1FETCH_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_SSBO_AOS_1FETCH_MODE].drawArrays.count = (GLuint)buddhaObj.Indices.size();
    drawCmd[PULLER_SSBO_AOS_3FETCH_MODE] = drawCmd[PULLER_SSBO_AOS_1FETCH_MODE];

    drawCmd[PULLER_SSBO_SOA_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_SSBO_SOA_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_SSBO_SOA_MODE].drawArrays.count = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_OBJ_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_OBJ_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_OBJ_MODE].drawArrays.count = (GLuint)buddhaObj.PositionIndices.size();

    drawCmd[PULLER_OBJ_SOFTCACHE_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_OBJ_SOFTCACHE_MODE].drawType = DRAWCMD_DRAWARRAYS;
    drawCmd[PULLER_OBJ_SOFTCACHE_MODE].drawArrays.count = (GLuint)buddhaObj.PositionIndices.size();

    drawCmd[GS_ASSEMBLER_MODE].vertexArray = assemblyVertexArray;
    drawCmd[GS_ASSEMBLER_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[GS_ASSEMBLER_MODE].primType = GL_TRIANGLES_ADJACENCY; // hack to get patches of 6 vertices
    drawCmd[GS_ASSEMBLER_MODE].drawElements.count = (GLuint)buddhaObj.PositionIndices.size() * 2;

    drawCmd[TS_ASSEMBLER_MODE].vertexArray = assemblyVertexArray;
    drawCmd[TS_ASSEMBLER_MODE].drawType = DRAWCMD_DRAWELEMENTS;
    drawCmd[TS_ASSEMBLER_MODE].primType = GL_PATCHES;
    drawCmd[TS_ASSEMBLER_MODE].patchVertices = 6;
    drawCmd[TS_ASSEMBLER_MODE].drawElements.count = (GLuint)buddhaObj.PositionIndices.size() * 2;

    // create auxiliary texture buffers
    glGenTextures(1, &indexTexBufferR32I);
    glBindTexture(GL_TEXTURE_BUFFER, indexTexBufferR32I);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, indexBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &positionTexBufferR32F);
    glBindTexture(GL_TEXTURE_BUFFER, positionTexBufferR32F);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, positionBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &normalTexBufferR32F);
    glBindTexture(GL_TEXTURE_BUFFER, normalTexBufferR32F);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, normalBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &positionTexBufferRGB32F);
    glBindTexture(GL_TEXTURE_BUFFER, positionTexBufferRGB32F);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, positionBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &normalTexBufferRGB32F);
    glBindTexture(GL_TEXTURE_BUFFER, normalTexBufferRGB32F);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, normalBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &positionTexBufferRGBA32F);
    glBindTexture(GL_TEXTURE_BUFFER, positionTexBufferRGBA32F);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, positionBufferXYZW);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &normalTexBufferRGBA32F);
    glBindTexture(GL_TEXTURE_BUFFER, normalTexBufferRGBA32F);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, normalBufferXYZW);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    for (size_t soaIdx = 0; soaIdx < 6; soaIdx++)
    {
        GLuint* pVertexBuffer =
            soaIdx == 0 ? &positionXBuffer        :
            soaIdx == 1 ? &positionYBuffer        :
            soaIdx == 2 ? &positionZBuffer        :
            soaIdx == 3 ? &normalXBuffer          :
            soaIdx == 4 ? &normalYBuffer          :
            soaIdx == 5 ? &normalZBuffer          :
            NULL;

        GLuint* pVertexTexBuffer =
            soaIdx == 0 ? &positionXTexBufferR32F :
            soaIdx == 1 ? &positionYTexBufferR32F :
            soaIdx == 2 ? &positionZTexBufferR32F :
            soaIdx == 3 ? &normalXTexBufferR32F   :
            soaIdx == 4 ? &normalYTexBufferR32F   :
            soaIdx == 5 ? &normalZTexBufferR32F   :
            NULL;

        GLenum format =
            soaIdx == 0 ? GL_R32F :
            soaIdx == 1 ? GL_R32F :
            soaIdx == 2 ? GL_R32F :
            soaIdx == 3 ? GL_R32F :
            soaIdx == 4 ? GL_R32F :
            soaIdx == 5 ? GL_R32F :
            NULL;

        glGenTextures(1, pVertexTexBuffer);
        glBindTexture(GL_TEXTURE_BUFFER, *pVertexTexBuffer);
        glTexBuffer(GL_TEXTURE_BUFFER, format, *pVertexBuffer);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }

    glGenBuffers(1, &vertexCacheBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexCacheBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, VERTEX_CACHE_VERTEX_SIZE_IN_DWORDS * sizeof(uint32_t) * buddhaObj.PositionIndices.size(), NULL, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

BuddhaDemo::BuddhaDemo()
{
    std::cout << "> Initializing renderer..." << std::endl;

    // initialize camera data
    cameraRotationFactor = 0.f;

    // create uniform buffer
    glGenBuffers(1, &transformUB);
    glBindBuffer(GL_UNIFORM_BUFFER, transformUB);
    glBufferStorage(GL_UNIFORM_BUFFER, sizeof(transform), NULL, GL_DYNAMIC_STORAGE_BIT);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenQueries(1, &timeElapsedQuery);

    loadShaders();

    glGenBuffers(1, &vertexCacheCounterBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexCacheCounterBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(GLuint), NULL, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vertexCacheMissCounterBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexCacheMissCounterBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(GLuint), NULL, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vertexCacheMissCounterReadbackBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexCacheMissCounterReadbackBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(GLuint), NULL, GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    SoftVertexCacheConfig cacheConfig;
    cacheConfig.NumCacheBucketBits = 20;
    cacheConfig.NumReadCacheLockAttempts = 1024;
    cacheConfig.NumWriteCacheLockAttempts = 1024;
    cacheConfig.NumCacheEntriesPerBucket = 2;
    cacheConfig.MaxSimultaneousReaders = 1000000; // this MUST be greater than the maximum concurrency of the GPU.
    cacheConfig.EnableCacheMissCounter = false;
    SetSoftVertexCacheConfig(cacheConfig);
}

int BuddhaDemo::addMesh(const char* path)
{
    PerModel model;
    model.load(path);
    models.push_back(model);
    return (int)models.size() - 1;
}

BuddhaDemo::VertexProg BuddhaDemo::loadShaderProgramFromFile(const char* filename, const char* preamble, GLenum shaderType)
{
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return {};
    }

    // weird C++ magic to read the file in one go
    std::string source(
        std::istreambuf_iterator<char>{file},
        std::istreambuf_iterator<char>{});

    if (file.bad()) {
        std::cerr << "Error reading the file: " << filename << std::endl;
        return {};
    }

    const GLchar* sources[] = {
        "#version 430 core\n",
        preamble ? preamble : "",
        source.c_str()
    };

    GLuint program = glCreateShaderProgramv(shaderType, sizeof(sources) / sizeof(*sources), sources);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        std::cerr << "Failed to compile/link shader program: " << filename << std::endl;
        GLchar log[10000];
        glGetProgramInfoLog(program, 10000, NULL, log);
        std::cerr << log << std::endl;
        exit(1);
    }

    return { program, filename };

}

GLuint BuddhaDemo::createProgramPipeline(GLuint vertexShader, GLuint tessControlShader, GLuint tessEvaluationShader, GLuint geometryShader, GLuint fragmentShader) {

    GLuint pipeline;
    glGenProgramPipelines(1, &pipeline);

    if (vertexShader != 0) glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, vertexShader);
    if (tessControlShader != 0) glUseProgramStages(pipeline, GL_TESS_CONTROL_SHADER, tessControlShader);
    if (tessEvaluationShader != 0) glUseProgramStages(pipeline, GL_TESS_EVALUATION_SHADER, tessEvaluationShader);
    if (geometryShader != 0) glUseProgramStages(pipeline, GL_GEOMETRY_SHADER_BIT, geometryShader);
    if (fragmentShader != 0) glUseProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, fragmentShader);

    glValidateProgramPipeline(pipeline);

    GLint status;
    glGetProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, &status);
    if (status != GL_TRUE) {
        std::cerr << "Failed to validate program pipeline:" << std::endl;
        GLchar log[10000];
        glGetProgramPipelineInfoLog(pipeline, 10000, NULL, log);
        std::cerr << log << std::endl;
        exit(1);
    }

    return pipeline;
}

void BuddhaDemo::loadShaders()
{
    std::cout << "> Loading shaders..." << std::endl;

    // load common fragment shader
    fragmentProg = loadShaderProgramFromFile("shaders/common.frag", 0, GL_FRAGMENT_SHADER).prog;

    vertexProg[FIXED_FUNCTION_AOS_MODE] = loadShaderProgramFromFile("shaders/fixed_aos.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FIXED_FUNCTION_AOS_MODE] = createProgramPipeline(vertexProg[FIXED_FUNCTION_AOS_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FIXED_FUNCTION_AOS_XYZW_MODE] = loadShaderProgramFromFile("shaders/fixed_aos.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FIXED_FUNCTION_AOS_XYZW_MODE] = createProgramPipeline(vertexProg[FIXED_FUNCTION_AOS_XYZW_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FIXED_FUNCTION_SOA_MODE] = loadShaderProgramFromFile("shaders/fixed_soa.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FIXED_FUNCTION_SOA_MODE] = createProgramPipeline(vertexProg[FIXED_FUNCTION_SOA_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FIXED_FUNCTION_INTERLEAVED_MODE] = loadShaderProgramFromFile("shaders/fixed_aos.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FIXED_FUNCTION_INTERLEAVED_MODE] = createProgramPipeline(vertexProg[FIXED_FUNCTION_INTERLEAVED_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_AOS_1RGBAFETCH_MODE] = loadShaderProgramFromFile("shaders/fetcher_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_AOS_1RGBAFETCH_MODE] = createProgramPipeline(vertexProg[FETCHER_AOS_1RGBAFETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_AOS_1RGBFETCH_MODE] = loadShaderProgramFromFile("shaders/fetcher_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_AOS_1RGBFETCH_MODE] = createProgramPipeline(vertexProg[FETCHER_AOS_1RGBFETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_AOS_3FETCH_MODE] = loadShaderProgramFromFile("shaders/fetcher_aos_3fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_AOS_3FETCH_MODE] = createProgramPipeline(vertexProg[FETCHER_AOS_3FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_SOA_MODE] = loadShaderProgramFromFile("shaders/fetcher_soa.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_SOA_MODE] = createProgramPipeline(vertexProg[FETCHER_SOA_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_IMAGE_AOS_1FETCH_MODE] = loadShaderProgramFromFile("shaders/fetcher_image_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_IMAGE_AOS_1FETCH_MODE] = createProgramPipeline(vertexProg[FETCHER_IMAGE_AOS_1FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_IMAGE_AOS_3FETCH_MODE] = loadShaderProgramFromFile("shaders/fetcher_image_aos_3fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_IMAGE_AOS_3FETCH_MODE] = createProgramPipeline(vertexProg[FETCHER_IMAGE_AOS_3FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_IMAGE_SOA_MODE] = loadShaderProgramFromFile("shaders/fetcher_image_soa.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_IMAGE_SOA_MODE] = createProgramPipeline(vertexProg[FETCHER_IMAGE_SOA_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_SSBO_AOS_1FETCH_MODE] = loadShaderProgramFromFile("shaders/fetcher_ssbo_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_SSBO_AOS_1FETCH_MODE] = createProgramPipeline(vertexProg[FETCHER_SSBO_AOS_1FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_SSBO_AOS_3FETCH_MODE] = loadShaderProgramFromFile("shaders/fetcher_ssbo_aos_3fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_SSBO_AOS_3FETCH_MODE] = createProgramPipeline(vertexProg[FETCHER_SSBO_AOS_3FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[FETCHER_SSBO_SOA_MODE] = loadShaderProgramFromFile("shaders/fetcher_ssbo_soa.vert", 0, GL_VERTEX_SHADER);
    progPipeline[FETCHER_SSBO_SOA_MODE] = createProgramPipeline(vertexProg[FETCHER_SSBO_SOA_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_AOS_1RGBAFETCH_MODE] = loadShaderProgramFromFile("shaders/puller_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_AOS_1RGBAFETCH_MODE] = createProgramPipeline(vertexProg[PULLER_AOS_1RGBAFETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_AOS_1RGBFETCH_MODE] = loadShaderProgramFromFile("shaders/puller_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_AOS_1RGBFETCH_MODE] = createProgramPipeline(vertexProg[PULLER_AOS_1RGBFETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_AOS_3FETCH_MODE] = loadShaderProgramFromFile("shaders/puller_aos_3fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_AOS_3FETCH_MODE] = createProgramPipeline(vertexProg[PULLER_AOS_3FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_SOA_MODE] = loadShaderProgramFromFile("shaders/puller_soa.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_SOA_MODE] = createProgramPipeline(vertexProg[PULLER_SOA_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_IMAGE_AOS_1FETCH_MODE] = loadShaderProgramFromFile("shaders/puller_image_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_IMAGE_AOS_1FETCH_MODE] = createProgramPipeline(vertexProg[PULLER_IMAGE_AOS_1FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_IMAGE_AOS_3FETCH_MODE] = loadShaderProgramFromFile("shaders/puller_image_aos_3fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_IMAGE_AOS_3FETCH_MODE] = createProgramPipeline(vertexProg[PULLER_IMAGE_AOS_3FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_IMAGE_SOA_MODE] = loadShaderProgramFromFile("shaders/puller_image_soa.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_IMAGE_SOA_MODE] = createProgramPipeline(vertexProg[PULLER_IMAGE_SOA_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_SSBO_AOS_1FETCH_MODE] = loadShaderProgramFromFile("shaders/puller_ssbo_aos_1fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_SSBO_AOS_1FETCH_MODE] = createProgramPipeline(vertexProg[PULLER_SSBO_AOS_1FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_SSBO_AOS_3FETCH_MODE] = loadShaderProgramFromFile("shaders/puller_ssbo_aos_3fetch.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_SSBO_AOS_3FETCH_MODE] = createProgramPipeline(vertexProg[PULLER_SSBO_AOS_3FETCH_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_SSBO_SOA_MODE] = loadShaderProgramFromFile("shaders/puller_ssbo_soa.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_SSBO_SOA_MODE] = createProgramPipeline(vertexProg[PULLER_SSBO_SOA_MODE], 0, 0, 0, fragmentProg);

    vertexProg[PULLER_OBJ_MODE] = loadShaderProgramFromFile("shaders/puller_obj.vert", 0, GL_VERTEX_SHADER);
    progPipeline[PULLER_OBJ_MODE] = createProgramPipeline(vertexProg[PULLER_OBJ_MODE], 0, 0, 0, fragmentProg);

    vertexProg[GS_ASSEMBLER_MODE] = loadShaderProgramFromFile("shaders/assembler.vert", 0, GL_VERTEX_SHADER);
    GLuint assemblyGeom = loadShaderProgramFromFile("shaders/gs_assembler.geom", 0, GL_GEOMETRY_SHADER).prog;
    progPipeline[GS_ASSEMBLER_MODE] = createProgramPipeline(vertexProg[GS_ASSEMBLER_MODE], 0, 0, assemblyGeom, fragmentProg);

    vertexProg[TS_ASSEMBLER_MODE] = loadShaderProgramFromFile("shaders/assembler.vert", 0, GL_VERTEX_SHADER);
    GLuint assemblyTesc = loadShaderProgramFromFile("shaders/ts_assembler.tesc", 0, GL_TESS_CONTROL_SHADER).prog;
    GLuint assemblyTese = loadShaderProgramFromFile("shaders/ts_assembler.tese", 0, GL_TESS_EVALUATION_SHADER).prog;
    progPipeline[TS_ASSEMBLER_MODE] = createProgramPipeline(vertexProg[TS_ASSEMBLER_MODE], assemblyTesc, assemblyTese, 0, fragmentProg);
}

SoftVertexCacheConfig BuddhaDemo::GetSoftVertexCacheConfig() const
{
    return softVertexCacheConfig;
}

void BuddhaDemo::SetSoftVertexCacheConfig(const SoftVertexCacheConfig& config)
{
    softVertexCacheConfig = config;

    std::string softcache_preamble =
        "#define NUM_CACHE_BUCKETS " + std::to_string(1 << (config.NumCacheBucketBits - 1)) + "\n" +
        "#define NUM_READ_CACHE_LOCK_ATTEMPTS " + std::to_string(config.NumReadCacheLockAttempts) + "\n" +
        "#define NUM_WRITE_CACHE_LOCK_ATTEMPTS " + std::to_string(config.NumWriteCacheLockAttempts) + "\n" +
        "#define NUM_CACHE_ENTRIES_PER_BUCKET " + std::to_string(config.NumCacheEntriesPerBucket) + "\n" +
        "#define MAX_SIMULTANEOUS_READERS " + std::to_string(config.MaxSimultaneousReaders) + "\n";

    if (config.EnableCacheMissCounter)
    {
        softcache_preamble += "#define ENABLE_CACHE_MISS_COUNTER\n";
    }

    glDeleteProgram(vertexProg[PULLER_OBJ_SOFTCACHE_MODE].prog);
    vertexProg[PULLER_OBJ_SOFTCACHE_MODE] = loadShaderProgramFromFile("shaders/puller_obj_softcache.vert", softcache_preamble.c_str(), GL_VERTEX_SHADER);
    
    glDeleteProgramPipelines(1, &progPipeline[PULLER_OBJ_SOFTCACHE_MODE]);
    progPipeline[PULLER_OBJ_SOFTCACHE_MODE] = createProgramPipeline(vertexProg[PULLER_OBJ_SOFTCACHE_MODE], 0, 0, 0, fragmentProg);

    GLsizei bucketSizeInBytes = VERTEX_CACHE_ENTRY_SIZE_IN_DWORDS * config.NumCacheEntriesPerBucket * sizeof(GLuint);
    GLsizei numBuckets = GLsizei(1 << (config.NumCacheBucketBits - 1));

    glDeleteBuffers(1, &vertexCacheBucketsBuffer);
    glGenBuffers(1, &vertexCacheBucketsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexCacheBucketsBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, bucketSizeInBytes * numBuckets, NULL, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDeleteBuffers(1, &vertexCacheBucketLocksBuffer);
    glGenBuffers(1, &vertexCacheBucketLocksBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexCacheBucketLocksBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(GLuint) * numBuckets, NULL, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void BuddhaDemo::renderScene(int meshID, const glm::mat4& modelMatrix, int screenWidth, int screenHeight, float dtsec, VertexPullingMode mode, uint64_t* elapsedNanoseconds)
{
	// update camera position
	cameraRotationFactor = fmodf(cameraRotationFactor + dtsec * 0.3f, 2.f * (float)M_PI);
	camera.position = glm::vec3(sin(cameraRotationFactor) * 5.f, 0.f, cos(cameraRotationFactor) * 5.f);
	camera.rotation = glm::vec3(0.f, -cameraRotationFactor, 0.f);

	// update camera data to uniform buffer
	transform.ModelViewMatrix = modelMatrix;
	transform.ModelViewMatrix = glm::rotate(transform.ModelViewMatrix, camera.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	transform.ModelViewMatrix = glm::rotate(transform.ModelViewMatrix, camera.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	transform.ModelViewMatrix = glm::rotate(transform.ModelViewMatrix, camera.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
	transform.ModelViewMatrix = translate(transform.ModelViewMatrix, -camera.position);
    transform.ProjectionMatrix = glm::perspective(glm::radians(45.0f), (float)screenWidth / screenHeight, 0.1f, 40.f);
	transform.MVPMatrix = transform.ProjectionMatrix * transform.ModelViewMatrix;
	glBindBuffer(GL_UNIFORM_BUFFER, transformUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(transform), &transform);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);

    glClearDepth(1.f);
    glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render scene
    glBindProgramPipeline(progPipeline[mode]);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);

    // glBindTextureUnit lookalike for GL 4.3
    auto bindBufferTextureUnit = [](GLuint unit, GLuint textureBuffer)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_BUFFER, textureBuffer);
    };

    PerModel& model = models[meshID];

    if (mode == FETCHER_AOS_1RGBAFETCH_MODE)
    {
        bindBufferTextureUnit(0, model.positionTexBufferRGBA32F);
        bindBufferTextureUnit(1, model.normalTexBufferRGBA32F);
    }
    else if (mode == FETCHER_AOS_1RGBFETCH_MODE)
    {
        bindBufferTextureUnit(0, model.positionTexBufferRGB32F);
        bindBufferTextureUnit(1, model.normalTexBufferRGB32F);
    }
    else if (mode == FETCHER_AOS_3FETCH_MODE)
    {
        bindBufferTextureUnit(0, model.positionTexBufferR32F);
        bindBufferTextureUnit(1, model.normalTexBufferR32F);
    }
    else if (mode == FETCHER_SOA_MODE)
    {
        bindBufferTextureUnit(0, model.positionXTexBufferR32F);
        bindBufferTextureUnit(1, model.positionYTexBufferR32F);
        bindBufferTextureUnit(2, model.positionZTexBufferR32F);
        bindBufferTextureUnit(3, model.normalXTexBufferR32F);
        bindBufferTextureUnit(4, model.normalYTexBufferR32F);
        bindBufferTextureUnit(5, model.normalZTexBufferR32F);
    }
    else if (mode == FETCHER_IMAGE_AOS_1FETCH_MODE)
    {
        glBindImageTexture(0, model.positionTexBufferRGBA32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, model.normalTexBufferRGBA32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    }
    else if (mode == FETCHER_IMAGE_AOS_3FETCH_MODE)
    {
        glBindImageTexture(0, model.positionTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(1, model.normalTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    else if (mode == FETCHER_IMAGE_SOA_MODE)
    {
        glBindImageTexture(0, model.positionXTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(1, model.positionYTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(2, model.positionZTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(3, model.normalXTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(4, model.normalYTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(5, model.normalZTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    else if (mode == FETCHER_SSBO_AOS_1FETCH_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.positionBufferXYZW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.normalBufferXYZW);
    }
    else if (mode == FETCHER_SSBO_AOS_3FETCH_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.positionBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.normalBuffer);
    }
    else if (mode == FETCHER_SSBO_SOA_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.positionXBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.positionYBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, model.positionZBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, model.normalXBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, model.normalYBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, model.normalZBuffer);
    }
    else if (mode == PULLER_AOS_1RGBAFETCH_MODE)
    {
        bindBufferTextureUnit(0, model.indexTexBufferR32I);
        bindBufferTextureUnit(1, model.positionTexBufferRGBA32F);
        bindBufferTextureUnit(2, model.normalTexBufferRGBA32F);
    }
    else if (mode == PULLER_AOS_1RGBFETCH_MODE)
    {
        bindBufferTextureUnit(0, model.indexTexBufferR32I);
        bindBufferTextureUnit(1, model.positionTexBufferRGB32F);
        bindBufferTextureUnit(2, model.normalTexBufferRGB32F);
    }
    else if (mode == PULLER_AOS_3FETCH_MODE)
    {
        bindBufferTextureUnit(0, model.indexTexBufferR32I);
        bindBufferTextureUnit(1, model.positionTexBufferR32F);
        bindBufferTextureUnit(2, model.normalTexBufferR32F);
    }
    else if (mode == PULLER_SOA_MODE)
    {
        bindBufferTextureUnit(0, model.indexTexBufferR32I);
        bindBufferTextureUnit(1, model.positionXTexBufferR32F);
        bindBufferTextureUnit(2, model.positionYTexBufferR32F);
        bindBufferTextureUnit(3, model.positionZTexBufferR32F);
        bindBufferTextureUnit(4, model.normalXTexBufferR32F);
        bindBufferTextureUnit(5, model.normalYTexBufferR32F);
        bindBufferTextureUnit(6, model.normalZTexBufferR32F);
    }
    else if (mode == PULLER_IMAGE_AOS_1FETCH_MODE)
    {
        glBindImageTexture(0, model.indexTexBufferR32I, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);
        glBindImageTexture(1, model.positionTexBufferRGBA32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(2, model.normalTexBufferRGBA32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    }
    else if (mode == PULLER_IMAGE_AOS_3FETCH_MODE)
    {
        glBindImageTexture(0, model.indexTexBufferR32I, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);
        glBindImageTexture(1, model.positionTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(2, model.normalTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    else if (mode == PULLER_IMAGE_SOA_MODE)
    {
        glBindImageTexture(0, model.indexTexBufferR32I, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);
        glBindImageTexture(1, model.positionXTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(2, model.positionYTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(3, model.positionZTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(4, model.normalXTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(5, model.normalYTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(6, model.normalZTexBufferR32F, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    else if (mode == PULLER_SSBO_AOS_1FETCH_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.indexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.positionBufferXYZW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, model.normalBufferXYZW);
    }
    else if (mode == PULLER_SSBO_AOS_3FETCH_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.indexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.positionBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, model.normalBuffer);
    }
    else if (mode == PULLER_SSBO_SOA_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.indexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.positionXBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, model.positionYBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, model.positionZBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, model.normalXBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, model.normalYBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, model.normalZBuffer);
    }
    else if (mode == PULLER_OBJ_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.positionIndexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.normalIndexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, model.uniquePositionBufferXYZW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, model.uniqueNormalBufferXYZW);
    }
    else if (mode == PULLER_OBJ_SOFTCACHE_MODE)
    {
        // make sure any previous reads/writes of these buffers are done before resetting them
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        const uint32_t kZero = 0;
        glBindBuffer(GL_ARRAY_BUFFER, vertexCacheCounterBuffer);
        glClearBufferData(GL_ARRAY_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &kZero);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        const uint32_t kInvalidAddress = -1;
        glBindBuffer(GL_ARRAY_BUFFER, vertexCacheBucketsBuffer);
        glClearBufferData(GL_ARRAY_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &kInvalidAddress);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        const uint32_t kUnlocked = 0;
        glBindBuffer(GL_ARRAY_BUFFER, vertexCacheBucketLocksBuffer);
        glClearBufferData(GL_ARRAY_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &kUnlocked);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.positionIndexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.normalIndexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, model.uniquePositionBufferXYZW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, model.uniqueNormalBufferXYZW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, vertexCacheCounterBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, model.vertexCacheBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, vertexCacheBucketsBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, vertexCacheBucketLocksBuffer);
        
        if (GetSoftVertexCacheConfig().EnableCacheMissCounter)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexCacheMissCounterBuffer);
            glClearBufferData(GL_ARRAY_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &kZero);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, vertexCacheMissCounterBuffer);
        }
    }
    else if (mode == GS_ASSEMBLER_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.uniquePositionBufferXYZW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.uniqueNormalBufferXYZW);
    }
    else if (mode == TS_ASSEMBLER_MODE)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model.uniquePositionBufferXYZW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, model.uniqueNormalBufferXYZW);
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, transformUB);

    glBindVertexArray(model.drawCmd[mode].vertexArray);

    if (model.drawCmd[mode].primType == GL_PATCHES)
    {
        assert(model.drawCmd[mode].patchVertices >= 1);

        glPatchParameteri(GL_PATCH_VERTICES, model.drawCmd[mode].patchVertices);
        glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, model.drawCmd[mode].patchDefaultOuterLevel);
        glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, model.drawCmd[mode].patchDefaultInnerLevel);
    }
    else
    {
        assert(model.drawCmd[mode].patchVertices == 0);
    }

    glBeginQuery(GL_TIME_ELAPSED, timeElapsedQuery);

    if (model.drawCmd[mode].drawType == DRAWCMD_DRAWARRAYS)
    {
        assert(model.drawCmd[mode].drawArrays.count >= 1);

        glDrawArraysInstancedBaseInstance(
            model.drawCmd[mode].primType,
            model.drawCmd[mode].drawArrays.first,
            model.drawCmd[mode].drawArrays.count,
            model.drawCmd[mode].drawArrays.instanceCount,
            model.drawCmd[mode].drawArrays.baseInstance);
    }
    else if (model.drawCmd[mode].drawType == DRAWCMD_DRAWELEMENTS)
    {
        glDrawElementsInstancedBaseVertexBaseInstance(
            model.drawCmd[mode].primType,
            model.drawCmd[mode].drawElements.count,
            GL_UNSIGNED_INT,
            (GLuint*)0 + model.drawCmd[mode].drawElements.firstIndex,
            model.drawCmd[mode].drawElements.instanceCount,
            model.drawCmd[mode].drawElements.baseVertex,
            model.drawCmd[mode].drawElements.baseInstance);
    }
    else
    {
        assert(!"invalid drawType");
    }

    glEndQuery(GL_TIME_ELAPSED);

    if (model.drawCmd[mode].primType == GL_PATCHES)
    {
        const float kDefaultInner[2] = { 1,1 };
        const float kDefaultOuter[4] = { 1,1,1,1 };

        glPatchParameteri(GL_PATCH_VERTICES, 3);
        glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, kDefaultOuter);
        glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, kDefaultInner);
    }

    glBindVertexArray(0);
    
    // unbind all resources to attempt to prevent GL from thinking one resource is being used in 2 ways simultaneously.
    for (int i = 0; i < 16; i++)
    {
        bindBufferTextureUnit(i, 0);
    }
    for (int i = 0; i < 16; i++)
    {
        glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    }
    for (int i = 0; i < 16; i++)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, 0);
    }

    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DEPTH_TEST);
    glBindProgramPipeline(0);

    if (elapsedNanoseconds)
        glGetQueryObjectui64v(timeElapsedQuery, GL_QUERY_RESULT, elapsedNanoseconds);

    if (mode == PULLER_OBJ_SOFTCACHE_MODE && GetSoftVertexCacheConfig().EnableCacheMissCounter)
    {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        glBindBuffer(GL_COPY_READ_BUFFER, vertexCacheMissCounterBuffer);
        glBindBuffer(GL_COPY_WRITE_BUFFER, vertexCacheMissCounterReadbackBuffer);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(GLuint));
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vertexCacheMissCounterReadbackBuffer);
        GLuint* pFinalCounter = (GLuint*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
        lastFrameNumVertexCacheMisses = *pFinalCounter;
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        lastFrameNumVertexCacheMisses = 0;
    }

    lastFrameMeshID = meshID;
}

} /* namespace buddha */
