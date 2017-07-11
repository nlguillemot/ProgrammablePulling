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

#include "wavefront.h"
#include "buddha.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#define MAIN_TITLE	"Programmable Pulling"

static int g_VertexPullingMode = buddha::FIXED_FUNCTION_AOS_MODE;

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

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, MAIN_TITLE, NULL, NULL);
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

    const char* modeStringFormats[buddha::NUMBER_OF_MODES]  = {};
    const char* modeStringHeader                             = "Flexibility        | Layout | Detail                  | GL object | Average time      ";
    modeStringFormats[buddha::FIXED_FUNCTION_AOS_MODE        ] = "Fixed-function     |   AoS  | One XYZ attrib          | VAO       | %8llu microseconds";
    modeStringFormats[buddha::FIXED_FUNCTION_AOS_XYZW_MODE   ] = "Fixed-function     |   AoS  | XYZ attrib w/ align(16) | VAO       | %8llu microseconds";
    modeStringFormats[buddha::FIXED_FUNCTION_SOA_MODE        ] = "Fixed-function     |   SoA  | Separate X/Y/Z attribs  | VAO       | %8llu microseconds";
    modeStringFormats[buddha::FIXED_FUNCTION_INTERLEAVED_MODE] = "Fixed-function     |   AoS  | Interleaved Pos/Normal  | VAO       | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_AOS_1RGBAFETCH_MODE    ] = "Programmable       |   AoS  | One RGBA32F texelFetch  | texture   | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_AOS_1RGBFETCH_MODE     ] = "Programmable       |   AoS  | One RGB32F texelFetch   | texture   | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_AOS_3FETCH_MODE        ] = "Programmable       |   AoS  | Three R32F texelFetch   | texture   | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_SOA_MODE               ] = "Programmable       |   SoA  | Three R32F texelFetch   | texture   | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_IMAGE_AOS_1FETCH_MODE  ] = "Programmable       |   AoS  | One RGBA32F imageLoad   | image     | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_IMAGE_AOS_3FETCH_MODE  ] = "Programmable       |   AoS  | Three R32F imageLoad    | image     | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_IMAGE_SOA_MODE         ] = "Programmable       |   SoA  | Three R32F imageLoad    | image     | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_SSBO_AOS_1FETCH_MODE   ] = "Programmable       |   AoS  | One RGBA32F SSBO load   | SSBO      | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_SSBO_AOS_3FETCH_MODE   ] = "Programmable       |   AoS  | Three R32F SSBO loads   | SSBO      | %8llu microseconds";
    modeStringFormats[buddha::FETCHER_SSBO_SOA_MODE          ] = "Programmable       |   SoA  | Three R32F SSBO loads   | SSBO      | %8llu microseconds";
    modeStringFormats[buddha::PULLER_AOS_1RGBAFETCH_MODE     ] = "Fully programmable |   AoS  | One RGBA32F texelFetch  | texture   | %8llu microseconds";
    modeStringFormats[buddha::PULLER_AOS_1RGBFETCH_MODE      ] = "Fully programmable |   AoS  | One RGB32F texelFetch   | texture   | %8llu microseconds";
    modeStringFormats[buddha::PULLER_AOS_3FETCH_MODE         ] = "Fully programmable |   AoS  | Three R32F texelFetch   | texture   | %8llu microseconds";
    modeStringFormats[buddha::PULLER_SOA_MODE                ] = "Fully programmable |   SoA  | Three R32F texelFetch   | texture   | %8llu microseconds";
    modeStringFormats[buddha::PULLER_IMAGE_AOS_1FETCH_MODE   ] = "Fully programmable |   AoS  | One RGBA32F imageLoad   | image     | %8llu microseconds";
    modeStringFormats[buddha::PULLER_IMAGE_AOS_3FETCH_MODE   ] = "Fully programmable |   AoS  | Three R32F imageLoad    | image     | %8llu microseconds";
    modeStringFormats[buddha::PULLER_IMAGE_SOA_MODE          ] = "Fully programmable |   SoA  | Three R32F imageLoad    | image     | %8llu microseconds";
    modeStringFormats[buddha::PULLER_SSBO_AOS_1FETCH_MODE    ] = "Fully programmable |   AoS  | One RGBA32F SSBO load   | SSBO      | %8llu microseconds";
    modeStringFormats[buddha::PULLER_SSBO_AOS_3FETCH_MODE    ] = "Fully programmable |   AoS  | Three R32F SSBO loads   | SSBO      | %8llu microseconds";
    modeStringFormats[buddha::PULLER_SSBO_SOA_MODE           ] = "Fully programmable |   SoA  | Three R32F SSBO loads   | SSBO      | %8llu microseconds";

    for (const char* mode : modeStringFormats)
    {
        assert(mode != NULL);
    }

    double then = glfwGetTime();
    bool animate = true;

    uint64_t totalTimes[buddha::NUMBER_OF_MODES] = {};
    int numTimes[buddha::NUMBER_OF_MODES] = {};

    static const int kFramesPerBenchmarkMode = 30;

    int currBenchmarkMode = 0;
    int currBenchmarkFrame = 0;
    bool nowBenchmarking = false;

    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);

    for (;;)
    {
        if (nowBenchmarking)
        {
            g_VertexPullingMode = currBenchmarkMode;
            
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
        pDemo->renderScene(animate ? (float)dtsec : 0.0f, (buddha::VertexPullingMode)g_VertexPullingMode, &elapsedNanoseconds);

        numTimes[g_VertexPullingMode] += 1;
        totalTimes[g_VertexPullingMode] += elapsedNanoseconds;

        ImGui::SetNextWindowSize(ImVec2(700.0f, 600.0f), ImGuiSetCond_Always);
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
                sprintf(modeStrings[i], modeStringFormats[i], lastTime / 1000);
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
            ImGui::ListBox("##Mode", &g_VertexPullingMode, items_getter, &getter_ctx, buddha::NUMBER_OF_MODES, buddha::NUMBER_OF_MODES);
            ImGui::PopItemWidth();

            ImGui::GetStyle().Colors[ImGuiCol_Text] = ImGuiStyle().Colors[ImGuiCol_Text];

            ImGui::Text("Frame time: %8llu microseconds", elapsedNanoseconds / 1000);
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
        }
        ImGui::End();

        // Render GUI
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        ImGui::Render();

       	glfwSwapBuffers(window);

        then = now;
	}
}
