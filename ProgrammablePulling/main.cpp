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

    const char* modeStrings[buddha::NUMBER_OF_MODES] =
    {
        "Fixed-function AoS vertex pulling",
        "Fixed-function SoA vertex pulling",
        "Programmable attribute AoS texture fetching",
        "Programmable attribute SoA texture fetching",
        "Programmable attribute AoS image fetching",
        "Programmable attribute SoA image fetching",
        "Fully programmable AoS vertex pulling",
        "Fully programmable SoA vertex pulling"
    };

    double then = glfwGetTime();

    for (;;)
    {
        double now = glfwGetTime();
        double dtsec = now - then;

        // fires any pending event callbacks
        glfwPollEvents();

        ImGui_ImplGlfwGL3_NewFrame();
        
        uint64_t elapsedNanoseconds;
        pDemo->renderScene((float)dtsec, (buddha::VertexPullingMode)g_VertexPullingMode, &elapsedNanoseconds);

        ImGui::SetNextWindowSize(ImVec2(500.0f, 200.0f), ImGuiSetCond_Always);
        if (ImGui::Begin("Info", 0, ImGuiWindowFlags_NoResize))
        {
            ImGui::ListBox("Mode", &g_VertexPullingMode, modeStrings, buddha::NUMBER_OF_MODES);
            ImGui::Text("Frame time: %8llu microseconds", elapsedNanoseconds / 1000);
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
