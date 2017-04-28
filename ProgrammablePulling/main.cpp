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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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

	buddha::BuddhaDemo demoScene;

	const char modeString[buddha::NUMBER_OF_MODES][100] =
	{
		"Fixed-function vertex pulling",
		"Programmable attribute fetching",
		"Fully programmable vertex pulling"
	};

    double then = glfwGetTime();
    uint64_t lastElapsedMilliseconds = -1;

	for (;;)
    {
        double now = glfwGetTime();
        double dtsec = now - then;

        buddha::VertexPullingMode oldMode = g_VertexPullingMode;

        // fires any pending event callbacks
        glfwPollEvents();
        
        uint64_t elapsedNanoseconds;
        demoScene.renderScene((float)dtsec, g_VertexPullingMode, &elapsedNanoseconds);

        uint64_t elapsedMicroseconds = elapsedNanoseconds / 1000;

        if (g_VertexPullingMode != oldMode || elapsedMicroseconds != lastElapsedMilliseconds)
        {
            char title[512];
            sprintf(title, "%s - %s (press SPACE to switch) - %llu microseconds", MAIN_TITLE, modeString[g_VertexPullingMode], elapsedMicroseconds);
            glfwSetWindowTitle(window, title);

            lastElapsedMilliseconds = elapsedMicroseconds;
        }

       	glfwSwapBuffers(window);

        then = now;
	}
}
