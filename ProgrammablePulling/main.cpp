/*
 * main.cpp
 *
 *  Created on: Sep 24, 2011
 *      Author: aqnuep
 */

#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>

#ifdef _WIN32
#include <d3d12.h>
#include <dxgi1_5.h>
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/gtx/transform.hpp>

#include "wavefront.h"
#include "buddha.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#define MAIN_TITLE	"Programmable Pulling"

static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
    exit(-1);
}

static void closeCallback(GLFWwindow*)
{
	exit(0);
}

#ifdef _WIN32
void SetStablePowerState()
{
    IDXGIFactory4* pDXGIFactory;
    CreateDXGIFactory1(IID_PPV_ARGS(&pDXGIFactory));

    IDXGIAdapter1* pDXGIAdapter;
    pDXGIFactory->EnumAdapters1(0, &pDXGIAdapter);

    ID3D12Device* pDevice;
    D3D12CreateDevice(pDXGIAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice));
    pDevice->SetStablePowerState(TRUE);
}
#else
void SetStablePowerState()
{
    // I don't know how to do this on other platforms.
}
#endif

int main() 
{
    // Set the GPU to a stable power state, in order to get reliable performance measurements.
    SetStablePowerState();

    glfwSetErrorCallback(errorCallback);

	if (glfwInit() != GL_TRUE) {
		std::cerr << "Error: unable to initialize GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    int screenWidth = DEFAULT_SCREEN_WIDTH;
    int screenHeight = DEFAULT_SCREEN_HEIGHT;

#ifdef _WIN32
    UINT dpi = GetDpiForSystem();
    screenWidth = MulDiv(screenWidth, dpi, 96);
    screenHeight = MulDiv(screenHeight, dpi, 96);
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(float(dpi) / 96.0f, float(dpi) / 96.0f);
#endif

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, MAIN_TITLE, NULL, NULL);
    if (!window) {
        std::cerr << "Error: unable to open GLFW window" << std::endl;
        return -1;
    }

    glfwSetWindowCloseCallback(window, closeCallback);

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum glewError;
	if ((glewError = glewInit()) != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(glewError) << std::endl;
		return -1;
	}

	if (!GLEW_VERSION_4_3) {
        std::cerr << "Error: OpenGL 4.3 is required" << std::endl;
		return -1;
	}

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true /* install callbacks */);

    std::shared_ptr<buddha::IBuddhaDemo> pDemo = buddha::IBuddhaDemo::Create();

    std::cout << "> Loading models..." << std::endl;

    std::vector<int> meshIDs;
    std::vector<std::string> meshDisplayNames;
    std::vector<glm::mat4> meshMatrices;

    meshDisplayNames.push_back("buddha");
    meshIDs.push_back(pDemo->addMesh("models/buddha.obj"));
    meshMatrices.push_back(glm::mat4());

    meshDisplayNames.push_back("cache optimized buddha");
    meshIDs.push_back(pDemo->addMesh("models/buddha-optimized.obj"));
    meshMatrices.push_back(glm::mat4());

    meshDisplayNames.push_back("sponza");
    meshIDs.push_back(pDemo->addMesh("models/sponza.obj"));
    meshMatrices.push_back(glm::translate(glm::vec3(0.0f, -1.0f, 0.0f)) * glm::scale(glm::vec3(1.0f / 100.0f)) * glm::rotate(glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f)));

    std::vector<const char*> meshDisplayNamesCStrs;
    for (const std::string& name : meshDisplayNames)
    {
        meshDisplayNamesCStrs.push_back(name.c_str());
    }

    int currMeshIndex = 0;

    const char* modeStringFormats[buddha::NUMBER_OF_MODES]  = {};
    const char* modeStringHeader                             =   "Programmability     | Layout | Detail                  | GL object | Average time       | Vertex shader";
    modeStringFormats[buddha::FIXED_FUNCTION_AOS_MODE        ] = "None (just VAO)     |   AoS  | One XYZ attrib          | VAO       | %8llu microseconds | %s";
    modeStringFormats[buddha::FIXED_FUNCTION_AOS_XYZW_MODE   ] = "None (just VAO)     |   AoS  | XYZ attrib w/ align(16) | VAO       | %8llu microseconds | %s";
    modeStringFormats[buddha::FIXED_FUNCTION_SOA_MODE        ] = "None (just VAO)     |   SoA  | Separate X/Y/Z attribs  | VAO       | %8llu microseconds | %s";
    modeStringFormats[buddha::FIXED_FUNCTION_INTERLEAVED_MODE] = "None (just VAO)     |   AoS  | Interleaved Pos/Normal  | VAO       | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_AOS_1RGBAFETCH_MODE    ] = "Pull vertex         |   AoS  | One RGBA32F texelFetch  | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_AOS_1RGBFETCH_MODE     ] = "Pull vertex         |   AoS  | One RGB32F texelFetch   | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_AOS_3FETCH_MODE        ] = "Pull vertex         |   AoS  | Three R32F texelFetch   | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_SOA_MODE               ] = "Pull vertex         |   SoA  | Three R32F texelFetch   | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_IMAGE_AOS_1FETCH_MODE  ] = "Pull vertex         |   AoS  | One RGBA32F imageLoad   | image     | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_IMAGE_AOS_3FETCH_MODE  ] = "Pull vertex         |   AoS  | Three R32F imageLoad    | image     | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_IMAGE_SOA_MODE         ] = "Pull vertex         |   SoA  | Three R32F imageLoad    | image     | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_SSBO_AOS_1FETCH_MODE   ] = "Pull vertex         |   AoS  | One RGBA32F SSBO load   | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_SSBO_AOS_3FETCH_MODE   ] = "Pull vertex         |   AoS  | Three R32F SSBO loads   | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::FETCHER_SSBO_SOA_MODE          ] = "Pull vertex         |   SoA  | Three R32F SSBO loads   | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_AOS_1RGBAFETCH_MODE     ] = "Pull index & vertex |   AoS  | One RGBA32F texelFetch  | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_AOS_1RGBFETCH_MODE      ] = "Pull index & vertex |   AoS  | One RGB32F texelFetch   | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_AOS_3FETCH_MODE         ] = "Pull index & vertex |   AoS  | Three R32F texelFetch   | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_SOA_MODE                ] = "Pull index & vertex |   SoA  | Three R32F texelFetch   | texture   | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_IMAGE_AOS_1FETCH_MODE   ] = "Pull index & vertex |   AoS  | One RGBA32F imageLoad   | image     | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_IMAGE_AOS_3FETCH_MODE   ] = "Pull index & vertex |   AoS  | Three R32F imageLoad    | image     | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_IMAGE_SOA_MODE          ] = "Pull index & vertex |   SoA  | Three R32F imageLoad    | image     | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_SSBO_AOS_1FETCH_MODE    ] = "Pull index & vertex |   AoS  | One RGBA32F SSBO load   | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_SSBO_AOS_3FETCH_MODE    ] = "Pull index & vertex |   AoS  | Three R32F SSBO loads   | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_SSBO_SOA_MODE           ] = "Pull index & vertex |   SoA  | Three R32F SSBO loads   | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_OBJ_MODE                ] = "Pull index & vertex |   AoS  | OBJ-style multi-index   | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::PULLER_OBJ_SOFTCACHE_MODE      ] = "Pull w/ soft cache  |   AoS  | OBJ-style + soft cache  | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::GS_ASSEMBLER_MODE              ] = "Assembly in GS      |   AoS  | OBJ-style + IA in GS    | SSBO      | %8llu microseconds | %s";
    modeStringFormats[buddha::TS_ASSEMBLER_MODE              ] = "Assembly in TS      |   AoS  | OBJ-style + IA in TS    | SSBO      | %8llu microseconds | %s";

    for (const char* mode : modeStringFormats)
    {
        assert(mode != NULL);
    }

    double then = glfwGetTime();
    bool animate = false;

    // asdf
    int numCacheBucketBits = 0;
    for (uint32_t i = 0; i <= 31; i++)
    {
        if ((1 << i) == DEFAULT_NUM_CACHE_BUCKETS)
        {
            numCacheBucketBits = i + 1;
        }
    }

    std::vector<std::vector<uint64_t>> meshTotalTimes;
    std::vector<std::vector<int>> meshNumTimes;
    for (size_t i = 0; i < meshIDs.size(); i++)
    {
        meshTotalTimes.push_back(std::vector<uint64_t>(buddha::NUMBER_OF_MODES, 0));
        meshNumTimes.push_back(std::vector<int>(buddha::NUMBER_OF_MODES, 0));
    }

    static const int kFramesPerBenchmarkMode = 30;

    int currBenchmarkMode = 0;
    int currBenchmarkFrame = 0;
    bool nowBenchmarking = false;

    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);

    int currDemoMode = buddha::FIXED_FUNCTION_AOS_MODE;

    for (;;)
    {
        if (nowBenchmarking)
        {
            currDemoMode = currBenchmarkMode;
            
            currBenchmarkFrame++;
            if (currBenchmarkFrame == kFramesPerBenchmarkMode)
            {
                currBenchmarkFrame = 0;
                currBenchmarkMode++;
                if (currBenchmarkMode == buddha::NUMBER_OF_MODES)
                {
                    currBenchmarkMode = 0;
                }
            }
        }

        double now = glfwGetTime();
        double dtsec = now - then;

        // fires any pending event callbacks
        glfwPollEvents();

        ImGui_ImplGlfwGL3_NewFrame();
        
        uint64_t elapsedNanoseconds;
        pDemo->renderScene(
            meshIDs[currMeshIndex],
            meshMatrices[currMeshIndex],
            screenWidth, screenHeight,
            animate ? (float)dtsec : 0.0f, 
            (buddha::VertexPullingMode)currDemoMode,
            &elapsedNanoseconds);

        uint64_t* totalTimes = meshTotalTimes[currMeshIndex].data();
        int* numTimes = meshNumTimes[currMeshIndex].data();

        numTimes[currDemoMode] += 1;
        totalTimes[currDemoMode] += elapsedNanoseconds;

        ImGui::SetNextWindowSize(ImVec2(900.0f, 700.0f), ImGuiSetCond_Always);
        if (ImGui::Begin("Info", 0, ImGuiWindowFlags_NoResize))
        {
            ImGui::Text("Vendor: %s", vendor);
            ImGui::Text("Renderer: %s", renderer);

            char modeStrings[buddha::NUMBER_OF_MODES][1024];
            const char* modeStringPtrs[buddha::NUMBER_OF_MODES];
            for (int i = 0; i < buddha::NUMBER_OF_MODES; i++)
            {
                uint64_t lastTime;
                lastTime = numTimes[i] == 0 ? 0 : totalTimes[i] / numTimes[i];
                
                std::string modeName = pDemo->GetModeName(i);
                static const std::string shaderPath = "shaders/";
                if (modeName.size() >= shaderPath.size() && modeName.substr(0, shaderPath.size()) == shaderPath)
                {
                    modeName = modeName.substr(shaderPath.size());
                }

                sprintf(modeStrings[i], modeStringFormats[i], lastTime / 1000, modeName.c_str());
                modeStringPtrs[i] = modeStrings[i];
            }

            struct items_getter_ctx
            {
                const char** modeStringPtrs;
                uint64_t* totalTimes;
                int* numTimes;
                uint64_t bestTime;
                uint64_t worstTime;
            };

            auto items_getter = [](void* ud, int choice, const char** out)
            {
                items_getter_ctx* ctx = (items_getter_ctx*)ud;

                ImGuiStyle& style = ImGui::GetStyle();

                if (ctx->numTimes[choice] == 0)
                {
                    style.Colors[ImGuiCol_Text] = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
                }
                else
                {
                    float lastTime = ctx->numTimes[choice] == 0 ? 0.0f : float(ctx->totalTimes[choice] / ctx->numTimes[choice]);
                    float goodness = 1.0f - (lastTime - float(ctx->bestTime)) / (float(ctx->worstTime) - float(ctx->bestTime));
                    style.Colors[ImGuiCol_Text] = ImColor(powf(1.0f - goodness, 1.0f / 2.2f), powf(goodness, 1.0f / 2.2f), 0.0f, 1.0f);
                }

                *out = ctx->modeStringPtrs[choice];

                return true;
            };

            items_getter_ctx getter_ctx;
            getter_ctx.modeStringPtrs = modeStringPtrs;
            getter_ctx.totalTimes = totalTimes;
            getter_ctx.numTimes = numTimes;
            getter_ctx.bestTime = 0;
            getter_ctx.worstTime = 0;

            for (int i = 0; i < buddha::NUMBER_OF_MODES; i++)
            {
                uint64_t lastTime;

                if (numTimes[i] == 0)
                {
                    lastTime = 0;
                }
                else
                {
                    lastTime = totalTimes[i] / numTimes[i];
                }

                if (getter_ctx.bestTime == 0 || lastTime < getter_ctx.bestTime)
                {
                    getter_ctx.bestTime = lastTime;
                }

                if (getter_ctx.worstTime == 0 || lastTime > getter_ctx.worstTime)
                {
                    getter_ctx.worstTime = lastTime;
                }
            }

            ImGui::PushItemWidth(-1);
            ImGui::Text(" %s", modeStringHeader);
            ImGui::ListBox("##Mode", &currDemoMode, items_getter, &getter_ctx, buddha::NUMBER_OF_MODES, buddha::NUMBER_OF_MODES);
            ImGui::PopItemWidth();

            ImGui::GetStyle().Colors[ImGuiCol_Text] = ImGuiStyle().Colors[ImGuiCol_Text];

            ImGui::Text("Frame time: %8llu microseconds", elapsedNanoseconds / 1000);

            ImGui::ListBox("Mesh", &currMeshIndex, meshDisplayNamesCStrs.data(), (int)meshDisplayNamesCStrs.size());

            ImGui::Checkbox("Animate", &animate);

            bool wasBenchmarking = nowBenchmarking;
            ImGui::Checkbox("Benchmark", &nowBenchmarking);

            if (ImGui::Button("Reset Benchmarks"))
            {
                for (int i = 0; i < buddha::NUMBER_OF_MODES; i++)
                {
                    totalTimes[i] = 0;
                    numTimes[i] = 0;
                }
            }

            if (currDemoMode == buddha::PULLER_OBJ_SOFTCACHE_MODE)
            {
                if (pDemo->GetSoftVertexCacheConfig().EnableCacheMissCounter)
                {
                    int numCacheMisses;
                    int totalNumVerts;
                    pDemo->GetSoftVertexCacheStats(&numCacheMisses, &totalNumVerts);

                    ImGui::Text("Num vertex cache misses: %d / %d", numCacheMisses, totalNumVerts);
                }

                buddha::SoftVertexCacheConfig cacheConfig = pDemo->GetSoftVertexCacheConfig();
                bool updatedConfig = false;
                updatedConfig |= ImGui::Checkbox("Count cache misses (affects perf.)", &cacheConfig.EnableCacheMissCounter);
                updatedConfig |= ImGui::SliderInt("Soft vertex cache bucket bits", &cacheConfig.NumCacheBucketBits, 1, 20);
                updatedConfig |= ImGui::SliderInt("Soft vertex cache read lock attempts", &cacheConfig.NumReadCacheLockAttempts, 0, 1024);
                updatedConfig |= ImGui::SliderInt("Soft vertex cache write lock attempts", &cacheConfig.NumWriteCacheLockAttempts, 0, 1024);
                updatedConfig |= ImGui::SliderInt("Soft vertex cache entries per bucket", &cacheConfig.NumCacheEntriesPerBucket, 1, 10);
                if (updatedConfig)
                {
                    pDemo->SetSoftVertexCacheConfig(cacheConfig);

                    totalTimes[buddha::PULLER_OBJ_SOFTCACHE_MODE] = 0;
                    numTimes[buddha::PULLER_OBJ_SOFTCACHE_MODE] = 0;
                }
            }
        }
        ImGui::End();

        // Render GUI
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
        ImGui::Render();

       	glfwSwapBuffers(window);

        then = now;
	}
}
