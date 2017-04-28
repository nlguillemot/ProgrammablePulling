/*
 * dragon.cpp
 *
 *  Created on: Sep 24, 2011
 *      Author: aqnuep
 */

#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "wavefront.h"
#include "buddha.h"

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

struct DrawCommand {
    GLuint vertexArray;
	bool useIndices;				// specifies whether this is an indexed draw command
	GLenum prim_type;				// primitive type
	union {
		struct {
			GLuint indexOffset;	// offset into the index buffer
			GLuint indexCount;	// number of indices
		};
		struct {
			GLuint firstVertex;	// first vertex index
			GLuint vertexCount;	// number of vertices
		};
	};
};

class BuddhaDemo : public IBuddhaDemo {
protected:

    Camera camera;                          // camera data

    Transform transform;                    // transformation data
    GLuint transformUB;                     // uniform buffer for the transformation

    GLuint fragmentProg;                    // common fragment shader program
    GLuint vertexProg[NUMBER_OF_MODES];     // vertex shader programs for the three vertex pulling modes
    GLuint progPipeline[NUMBER_OF_MODES];   // program pipelines for the three vertex pulling modes

    GLuint indexBuffer;                     // index buffer for the mesh
    GLuint positionBuffer;                    // vertex buffer for the mesh
    GLuint normalBuffer;

    GLuint positionXBuffer;
    GLuint positionYBuffer;
    GLuint positionZBuffer;
    GLuint normalXBuffer;
    GLuint normalYBuffer;
    GLuint normalZBuffer;

    GLuint nullVertexArray;
    GLuint vertexArrayIndexBufferOnly;
    GLuint vertexArrayAoS;                     // vertex array for the three vertex pulling modes
    GLuint vertexArraySoA;                     // vertex array for the three vertex pulling modes

    GLuint indexTexBuffer;                  // index buffer texture
    GLuint positionTexBuffer;                 
    GLuint normalTexBuffer;

    GLuint positionXTexBuffer;
    GLuint positionYTexBuffer;
    GLuint positionZTexBuffer;
    GLuint normalXTexBuffer;
    GLuint normalYTexBuffer;
    GLuint normalZTexBuffer;

    GLuint timeElapsedQuery;                // query object for the time taken to render the scene

    DrawCommand drawCmd[NUMBER_OF_MODES];   // draw command for the three vertex pulling modes

    float cameraRotationFactor;             // camera rotation factor between [0,2*PI)

    void loadModels();
    void loadShaders();

    GLuint loadShaderProgramFromFile(const char* filename, GLenum shaderType);
    GLuint createProgramPipeline(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader);

public:
    BuddhaDemo();

    void renderScene(float dtsec, VertexPullingMode mode, uint64_t* elapsedNanoseconds);
};

std::shared_ptr<IBuddhaDemo> IBuddhaDemo::Create()
{
    return std::make_shared<BuddhaDemo>();
}

BuddhaDemo::BuddhaDemo() {

	std::cout << "> Initializing scene data..." << std::endl;

	// initialize camera data
	cameraRotationFactor = 0.f;
	transform.ProjectionMatrix = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 40.f);

	// create uniform buffer and store camera data
	glGenBuffers(1, &transformUB);
	glBindBuffer(GL_UNIFORM_BUFFER, transformUB);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(transform), &transform, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenQueries(1, &timeElapsedQuery);

	loadShaders();
	loadModels();

    std::cout << "> Done!" << std::endl;
}

void BuddhaDemo::loadModels() {

    std::cout << "> Loading models..." << std::endl;

	demo::WaveFrontObj buddhaObj("models/buddha.obj");

    std::cout << "> Uploading mesh data to GPU..." << std::endl;

	// create index buffer
	GLsizei indexBufferSize = (GLsizei)(buddhaObj.Indices.size() * sizeof(GLuint));
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

	// map index buffer and fill with data
    GLuint *index = (GLuint*)glMapBufferRange(GL_ARRAY_BUFFER, 0, indexBufferSize,
						GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    for (size_t i = 0; i < buddhaObj.Indices.size(); i++)
		index[i] = buddhaObj.Indices[i];

	glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

	// create AoS vertex buffer
    GLsizei positionBufferSize = (GLsizei)(buddhaObj.Positions.size() * sizeof(glm::vec3));
	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, positionBufferSize, NULL, GL_STATIC_DRAW);

	// map vertex buffer and fill with data
	glm::vec3* positions = (glm::vec3*)glMapBufferRange(GL_ARRAY_BUFFER, 0, positionBufferSize,
						GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    for (size_t i = 0; i < buddhaObj.Positions.size(); i++)
    {
        positions[i][0] = buddhaObj.Positions[i][0];
        positions[i][1] = buddhaObj.Positions[i][1];
        positions[i][2] = buddhaObj.Positions[i][2];
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLsizei normalBufferSize = (GLsizei)(buddhaObj.Normals.size() * sizeof(glm::vec3));
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, normalBufferSize, NULL, GL_STATIC_DRAW);

    // map vertex buffer and fill with data
    glm::vec3* normals = (glm::vec3*)glMapBufferRange(GL_ARRAY_BUFFER, 0, normalBufferSize,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    for (size_t i = 0; i < buddhaObj.Normals.size(); i++)
    {
        normals[i][0] = buddhaObj.Normals[i][0];
        normals[i][1] = buddhaObj.Normals[i][1];
        normals[i][2] = buddhaObj.Normals[i][2];
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // SoA buffers
    for (size_t soaIdx = 0; soaIdx < 6; soaIdx++)
    {
        GLuint* pVertexBuffer =
            soaIdx == 0 ? &positionXBuffer :
            soaIdx == 1 ? &positionYBuffer :
            soaIdx == 2 ? &positionZBuffer :
            soaIdx == 3 ? &normalXBuffer   :
            soaIdx == 4 ? &normalYBuffer   :
            soaIdx == 5 ? &normalZBuffer   :
            NULL;

        // create empty vertex buffer
        GLsizei vertexBufferSize = (GLsizei)(buddhaObj.Positions.size() * sizeof(float));
        glGenBuffers(1, pVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

        // map vertex buffer and fill with data
        float* buf = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, vertexBufferSize,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

        for (size_t i = 0; i < buddhaObj.Positions.size(); i++) {
            buf[i] =
                soaIdx == 0 ? buddhaObj.Positions[i].x :
                soaIdx == 1 ? buddhaObj.Positions[i].y :
                soaIdx == 2 ? buddhaObj.Positions[i].z :
                soaIdx == 3 ? buddhaObj.Normals[i].x   :
                soaIdx == 4 ? buddhaObj.Normals[i].y   :
                soaIdx == 5 ? buddhaObj.Normals[i].z   :
                0.0f;
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
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

    drawCmd[FIXED_FUNCTION_AOS_MODE].vertexArray = vertexArrayAoS;
	drawCmd[FIXED_FUNCTION_AOS_MODE].useIndices = true;
	drawCmd[FIXED_FUNCTION_AOS_MODE].prim_type = GL_TRIANGLES;
	drawCmd[FIXED_FUNCTION_AOS_MODE].indexOffset = 0;
	drawCmd[FIXED_FUNCTION_AOS_MODE].indexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[FIXED_FUNCTION_SOA_MODE].vertexArray = vertexArraySoA;
    drawCmd[FIXED_FUNCTION_SOA_MODE].useIndices = true;
    drawCmd[FIXED_FUNCTION_SOA_MODE].prim_type = GL_TRIANGLES;
    drawCmd[FIXED_FUNCTION_SOA_MODE].indexOffset = 0;
    drawCmd[FIXED_FUNCTION_SOA_MODE].indexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[FETCHER_AOS_MODE].vertexArray = vertexArrayIndexBufferOnly;
	drawCmd[FETCHER_AOS_MODE].useIndices = true;
	drawCmd[FETCHER_AOS_MODE].prim_type = GL_TRIANGLES;
	drawCmd[FETCHER_AOS_MODE].indexOffset = 0;
	drawCmd[FETCHER_AOS_MODE].indexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[FETCHER_SOA_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_SOA_MODE].useIndices = true;
    drawCmd[FETCHER_SOA_MODE].prim_type = GL_TRIANGLES;
    drawCmd[FETCHER_SOA_MODE].indexOffset = 0;
    drawCmd[FETCHER_SOA_MODE].indexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[FETCHER_IMAGE_AOS_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_IMAGE_AOS_MODE].useIndices = true;
    drawCmd[FETCHER_IMAGE_AOS_MODE].prim_type = GL_TRIANGLES;
    drawCmd[FETCHER_IMAGE_AOS_MODE].indexOffset = 0;
    drawCmd[FETCHER_IMAGE_AOS_MODE].indexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[FETCHER_IMAGE_SOA_MODE].vertexArray = vertexArrayIndexBufferOnly;
    drawCmd[FETCHER_IMAGE_SOA_MODE].useIndices = true;
    drawCmd[FETCHER_IMAGE_SOA_MODE].prim_type = GL_TRIANGLES;
    drawCmd[FETCHER_IMAGE_SOA_MODE].indexOffset = 0;
    drawCmd[FETCHER_IMAGE_SOA_MODE].indexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_AOS_MODE].vertexArray = nullVertexArray;
	drawCmd[PULLER_AOS_MODE].useIndices = false;
	drawCmd[PULLER_AOS_MODE].prim_type = GL_TRIANGLES;
	drawCmd[PULLER_AOS_MODE].firstVertex = 0;
	drawCmd[PULLER_AOS_MODE].vertexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_SOA_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_SOA_MODE].useIndices = false;
    drawCmd[PULLER_SOA_MODE].prim_type = GL_TRIANGLES;
    drawCmd[PULLER_SOA_MODE].firstVertex = 0;
    drawCmd[PULLER_SOA_MODE].vertexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_IMAGE_AOS_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_IMAGE_AOS_MODE].useIndices = false;
    drawCmd[PULLER_IMAGE_AOS_MODE].prim_type = GL_TRIANGLES;
    drawCmd[PULLER_IMAGE_AOS_MODE].firstVertex = 0;
    drawCmd[PULLER_IMAGE_AOS_MODE].vertexCount = (GLuint)buddhaObj.Indices.size();

    drawCmd[PULLER_IMAGE_SOA_MODE].vertexArray = nullVertexArray;
    drawCmd[PULLER_IMAGE_SOA_MODE].useIndices = false;
    drawCmd[PULLER_IMAGE_SOA_MODE].prim_type = GL_TRIANGLES;
    drawCmd[PULLER_IMAGE_SOA_MODE].firstVertex = 0;
    drawCmd[PULLER_IMAGE_SOA_MODE].vertexCount = (GLuint)buddhaObj.Indices.size();

	// create auxiliary texture buffers
	glGenTextures(1, &indexTexBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, indexTexBuffer);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, indexBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

	glGenTextures(1, &positionTexBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, positionTexBuffer);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, positionBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenTextures(1, &normalTexBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, normalTexBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, normalBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    for (size_t soaIdx = 0; soaIdx < 6; soaIdx++)
    {
        GLuint* pVertexBuffer =
            soaIdx == 0 ? &positionXBuffer :
            soaIdx == 1 ? &positionYBuffer :
            soaIdx == 2 ? &positionZBuffer :
            soaIdx == 3 ? &normalXBuffer :
            soaIdx == 4 ? &normalYBuffer :
            soaIdx == 5 ? &normalZBuffer :
            NULL;

        GLuint* pVertexTexBuffer =
            soaIdx == 0 ? &positionXTexBuffer :
            soaIdx == 1 ? &positionYTexBuffer :
            soaIdx == 2 ? &positionZTexBuffer :
            soaIdx == 3 ? &normalXTexBuffer :
            soaIdx == 4 ? &normalYTexBuffer :
            soaIdx == 5 ? &normalZTexBuffer :
            NULL;

        glGenTextures(1, pVertexTexBuffer);
        glBindTexture(GL_TEXTURE_BUFFER, *pVertexTexBuffer);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, *pVertexBuffer);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }
}

GLuint BuddhaDemo::loadShaderProgramFromFile(const char* filename, GLenum shaderType)
{
	std::ifstream file(filename);
	if (!file) {
		std::cerr << "Unable to open file: " << filename << std::endl;
		return 0;
	}

    // weird C++ magic to read the file in one go
    std::string source(
        std::istreambuf_iterator<char>{file},
        std::istreambuf_iterator<char>{});

	if (file.bad()) {
        std::cerr << "Error reading the file: " << filename << std::endl;
		return 0;
	}

    const GLchar* sources[] = {
        "#version 430 core\n",
        source.c_str()
    };

    GLuint program = glCreateShaderProgramv(shaderType, sizeof(sources) / sizeof(*sources), sources);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
        std::cerr << "Failed to compile/link shader program:" << std::endl;
		GLchar log[10000];
		glGetProgramInfoLog(program, 10000, NULL, log);
        std::cerr << log << std::endl;
		exit(1);
	}

	return program;

}

GLuint BuddhaDemo::createProgramPipeline(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader) {

	GLuint pipeline;
	glGenProgramPipelines(1, &pipeline);

	if (vertexShader != 0) glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, vertexShader);
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

void BuddhaDemo::loadShaders() {

    std::cout << "> Loading shaders..." << std::endl;

	// load common fragment shader
	fragmentProg = loadShaderProgramFromFile("shaders/common.frag", GL_FRAGMENT_SHADER);

	vertexProg[FIXED_FUNCTION_AOS_MODE] = loadShaderProgramFromFile("shaders/fixed_aos.vert", GL_VERTEX_SHADER);
	progPipeline[FIXED_FUNCTION_AOS_MODE] = createProgramPipeline(vertexProg[FIXED_FUNCTION_AOS_MODE], 0, fragmentProg);

    vertexProg[FIXED_FUNCTION_SOA_MODE] = loadShaderProgramFromFile("shaders/fixed_soa.vert", GL_VERTEX_SHADER);
    progPipeline[FIXED_FUNCTION_SOA_MODE] = createProgramPipeline(vertexProg[FIXED_FUNCTION_SOA_MODE], 0, fragmentProg);

	vertexProg[FETCHER_AOS_MODE] = loadShaderProgramFromFile("shaders/fetcher_aos.vert", GL_VERTEX_SHADER);
	progPipeline[FETCHER_AOS_MODE] = createProgramPipeline(vertexProg[FETCHER_AOS_MODE], 0, fragmentProg);

    vertexProg[FETCHER_SOA_MODE] = loadShaderProgramFromFile("shaders/fetcher_soa.vert", GL_VERTEX_SHADER);
    progPipeline[FETCHER_SOA_MODE] = createProgramPipeline(vertexProg[FETCHER_SOA_MODE], 0, fragmentProg);

    vertexProg[FETCHER_IMAGE_AOS_MODE] = loadShaderProgramFromFile("shaders/fetcher_image_aos.vert", GL_VERTEX_SHADER);
    progPipeline[FETCHER_IMAGE_AOS_MODE] = createProgramPipeline(vertexProg[FETCHER_IMAGE_AOS_MODE], 0, fragmentProg);

    vertexProg[FETCHER_IMAGE_SOA_MODE] = loadShaderProgramFromFile("shaders/fetcher_image_soa.vert", GL_VERTEX_SHADER);
    progPipeline[FETCHER_IMAGE_SOA_MODE] = createProgramPipeline(vertexProg[FETCHER_IMAGE_SOA_MODE], 0, fragmentProg);

	vertexProg[PULLER_AOS_MODE] = loadShaderProgramFromFile("shaders/puller_aos.vert", GL_VERTEX_SHADER);
	progPipeline[PULLER_AOS_MODE] = createProgramPipeline(vertexProg[PULLER_AOS_MODE], 0, fragmentProg);

    vertexProg[PULLER_SOA_MODE] = loadShaderProgramFromFile("shaders/puller_soa.vert", GL_VERTEX_SHADER);
    progPipeline[PULLER_SOA_MODE] = createProgramPipeline(vertexProg[PULLER_SOA_MODE], 0, fragmentProg);

    vertexProg[PULLER_IMAGE_AOS_MODE] = loadShaderProgramFromFile("shaders/puller_image_aos.vert", GL_VERTEX_SHADER);
    progPipeline[PULLER_IMAGE_AOS_MODE] = createProgramPipeline(vertexProg[PULLER_IMAGE_AOS_MODE], 0, fragmentProg);

    vertexProg[PULLER_IMAGE_SOA_MODE] = loadShaderProgramFromFile("shaders/puller_image_soa.vert", GL_VERTEX_SHADER);
    progPipeline[PULLER_IMAGE_SOA_MODE] = createProgramPipeline(vertexProg[PULLER_IMAGE_SOA_MODE], 0, fragmentProg);
}

void BuddhaDemo::renderScene(float dtsec, VertexPullingMode mode, uint64_t* elapsedNanoseconds)
{
	// update camera position
	cameraRotationFactor = fmodf(cameraRotationFactor + dtsec * 0.3f, 2.f * (float)M_PI);
	camera.position = glm::vec3(sin(cameraRotationFactor) * 5.f, 0.f, cos(cameraRotationFactor) * 5.f);
	camera.rotation = glm::vec3(0.f, -cameraRotationFactor, 0.f);

	// update camera data to uniform buffer
	transform.ModelViewMatrix = glm::mat4(1.0f);
	transform.ModelViewMatrix = glm::rotate(transform.ModelViewMatrix, camera.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	transform.ModelViewMatrix = glm::rotate(transform.ModelViewMatrix, camera.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	transform.ModelViewMatrix = glm::rotate(transform.ModelViewMatrix, camera.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
	transform.ModelViewMatrix = translate(transform.ModelViewMatrix, -camera.position);
	transform.MVPMatrix = transform.ProjectionMatrix * transform.ModelViewMatrix;
	glBindBuffer(GL_UNIFORM_BUFFER, transformUB);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(transform), &transform, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glClearDepth(1.f);
    glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBeginQuery(GL_TIME_ELAPSED, timeElapsedQuery);

    // render scene
    glBindProgramPipeline(progPipeline[mode]);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);

    // glBindTextureUnit lookalike for GL 4.3
    auto bindBufferTextureUnit = [](GLuint unit, GLuint textureBuffer)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_BUFFER, textureBuffer);
    };

    if (mode == FETCHER_AOS_MODE)
    {
        bindBufferTextureUnit(0, positionTexBuffer);
        bindBufferTextureUnit(1, normalTexBuffer);
    }
    else if (mode == FETCHER_SOA_MODE)
    {
        bindBufferTextureUnit(0, positionXTexBuffer);
        bindBufferTextureUnit(1, positionYTexBuffer);
        bindBufferTextureUnit(2, positionZTexBuffer);
        bindBufferTextureUnit(3, normalXTexBuffer);
        bindBufferTextureUnit(4, normalYTexBuffer);
        bindBufferTextureUnit(5, normalZTexBuffer);
    }
    else if (mode == FETCHER_IMAGE_AOS_MODE)
    {
        glBindImageTexture(0, positionTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(1, normalTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    else if (mode == FETCHER_IMAGE_SOA_MODE)
    {
        glBindImageTexture(0, positionXTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(1, positionYTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(2, positionZTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(3, normalXTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(4, normalYTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(5, normalZTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    else if (mode == PULLER_AOS_MODE)
    {
        bindBufferTextureUnit(0, indexTexBuffer);
        bindBufferTextureUnit(1, positionTexBuffer);
        bindBufferTextureUnit(2, normalTexBuffer);
    }
    else if (mode == PULLER_SOA_MODE)
    {
        bindBufferTextureUnit(0, indexTexBuffer);
        bindBufferTextureUnit(1, positionXTexBuffer);
        bindBufferTextureUnit(2, positionYTexBuffer);
        bindBufferTextureUnit(3, positionZTexBuffer);
        bindBufferTextureUnit(4, normalXTexBuffer);
        bindBufferTextureUnit(5, normalYTexBuffer);
        bindBufferTextureUnit(6, normalZTexBuffer);
    }
    else if (mode == PULLER_IMAGE_AOS_MODE)
    {
        glBindImageTexture(0, indexTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);
        glBindImageTexture(1, positionTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(2, normalTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    else if (mode == PULLER_IMAGE_SOA_MODE)
    {
        glBindImageTexture(0, indexTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);
        glBindImageTexture(1, positionXTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(2, positionYTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(3, positionZTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(4, normalXTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(5, normalYTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(6, normalZTexBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, transformUB);

    glBindVertexArray(drawCmd[mode].vertexArray);

	if (drawCmd[mode].useIndices) {
		glDrawElements(drawCmd[mode].prim_type, drawCmd[mode].indexCount, GL_UNSIGNED_INT, (GLchar*)0 + drawCmd[mode].indexOffset);
	} else {
		glDrawArrays(drawCmd[mode].prim_type, drawCmd[mode].firstVertex, drawCmd[mode].vertexCount);
	}

    glBindVertexArray(0);

    glDisable(GL_CULL_FACE);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DEPTH_TEST);
    glBindProgramPipeline(0);

    glEndQuery(GL_TIME_ELAPSED);

    if (elapsedNanoseconds)
        glGetQueryObjectui64v(timeElapsedQuery, GL_QUERY_RESULT, elapsedNanoseconds);
}

} /* namespace buddha */
