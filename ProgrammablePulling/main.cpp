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
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "wavefront.h"
#include "buddha.h"

#define MAIN_TITLE	"OpenGL 4.3 Buddha demo"

static buddha::VertexPullingMode g_VertexPullingMode = buddha::FFX_MODE;

void closeCallback(GLFWwindow*)
{
	exit(0);
}

void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        exit(0);
    }


    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_VertexPullingMode = (buddha::VertexPullingMode)(((int)g_VertexPullingMode + 1) % buddha::NUMBER_OF_MODES);
    }
}

int main() {

	if (glfwInit() != GL_TRUE) {
		std::cerr << "Error: unable to initialize GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
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
    glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);

    glfwShowWindow(window);

	GLenum glewError;
	if ((glewError = glewInit()) != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(glewError) << std::endl;
		return -1;
	}

	if (!GLEW_VERSION_4_3) {
        std::cerr << "Error: OpenGL 4.3 is required" << std::endl;
		return -1;
	}

	double frameTime = 0.01;
	double lastFrameEndTime = glfwGetTime();
	double lastFPSCaptureTime = glfwGetTime();
    double currFPS = 0.0;
	int framesSinceLastFPSCapture = 0;

	buddha::BuddhaDemo demoScene;

	const char modeString[buddha::NUMBER_OF_MODES][100] =
	{
		"Fixed-function vertex pulling",
		"Programmable attribute fetching",
		"Fully programmable vertex pulling"
	};

	while (true)
    {
        buddha::VertexPullingMode oldMode = g_VertexPullingMode;
        double oldFPS = currFPS;

        // fires any pending event callbacks
        glfwPollEvents();
        
        demoScene.renderScene((float)frameTime, g_VertexPullingMode);

        framesSinceLastFPSCapture++;

        // count the number of frames that passed in each second
		if (glfwGetTime() - lastFPSCaptureTime > 1.0) {
            currFPS = (double)framesSinceLastFPSCapture / (glfwGetTime() - lastFPSCaptureTime);
            framesSinceLastFPSCapture = 0;
            lastFPSCaptureTime = glfwGetTime();
		}

        if (g_VertexPullingMode != oldMode || currFPS != oldFPS)
        {
            char title[512];
            sprintf(title, "%s - %s (press SPACE to switch) - %.2lf fps", MAIN_TITLE, modeString[g_VertexPullingMode], currFPS);
            glfwSetWindowTitle(window, title);
        }

       	glfwSwapBuffers(window);

       	double newFrameEnd = glfwGetTime();
       	frameTime = newFrameEnd - lastFrameEndTime;
        lastFrameEndTime = newFrameEnd;
	}
}
