#include "pch.hpp"

/* glad + glfw */
#include "gl33.h"
#include "glfw3.h"

unsigned int           W_WIDTH       = 1024;
unsigned int           W_HEIGHT      = 512;
bool                   IS_PAUSED     = false;
double                 D_TIME        = 0;
std::array<GLfloat,16> PROJECTION;

#include "gl.hpp"
#include "lin.hpp"
#include "cam.hpp"

#define u32 unsigned int
#define elif else if


bool wasKeyPressedOnce[349] = { false };
bool IsKeyInState (GLFWwindow* window, int code, int desiredState) {
    int currentKeyState = glfwGetKey(window, code);
    if (currentKeyState == desiredState && desiredState == GLFW_PRESS) {
        if (!wasKeyPressedOnce[code]) {
            wasKeyPressedOnce[code] = true;
            return true;
        }
    }
    elif (currentKeyState == desiredState && desiredState == GLFW_RELEASE) {
        if (wasKeyPressedOnce[code]) {
            wasKeyPressedOnce[code] = false;
            return true;
        }
    }
    elif (currentKeyState == GLFW_PRESS && desiredState == GLFW_REPEAT) {
        return true;
    }

    if (currentKeyState == GLFW_RELEASE) { wasKeyPressedOnce[code] = false; }
    elif (currentKeyState == GLFW_PRESS) { wasKeyPressedOnce[code] = true; }

    return false;
}


void windowRefreshCallback (GLFWwindow *window) {
    //render();
    //glfwSwapBuffers(window);
    //glFinish(); // important, this waits until rendering result is actually visible, thus making resizing less ugly
}

void framebufferSizeCallback (GLFWwindow *window, int width, int height) {
    W_WIDTH = width;
    W_HEIGHT = height;
    PROJECTION[0] = 2.0f/W_WIDTH; PROJECTION[5] = 2.0f/W_HEIGHT;
    // gl::UpdateProjectionUniforms();
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
}

void scrollCallback (GLFWwindow* window, double xoffset, double yoffset) {
    PROJECTION = lin::Translate(-10 * xoffset/W_WIDTH, -10 * yoffset/W_HEIGHT) * PROJECTION;
}

int main () {
    if (!glfwInit()) { return -1; }

    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "test", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    // glfwSetWindowPos(window, 0, 30); 

    gladLoadGL(glfwGetProcAddress);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
    glEnable(GL_MULTISAMPLE);
    // glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CW); /* doesn't render the back face of objects, faster rendering? indices would need to be clockwise */

    PROJECTION = {
        2.0f/W_WIDTH, 0.0f,          0.0f, 0.0f,
        0.0f,         2.0f/W_HEIGHT, 0.0f, 0.0f,
        0.0f,         0.0f,          1.0f, 0.0f,
        0.0f,         0.0f,          0.0f, 1.0f
    };

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
   
    
    
    while (!glfwWindowShouldClose(window)) {
        D_TIME = glfwGetTime();
        glfwSetTime(0.0);

        if (IsKeyInState(window, GLFW_KEY_P, GLFW_PRESS)) { /* pause */
            IS_PAUSED = !IS_PAUSED;
        }
        if (IsKeyInState(window, GLFW_KEY_Q, GLFW_PRESS) || IsKeyInState(window, GLFW_KEY_ESCAPE, GLFW_PRESS)) { /* quit */
            break;
        }

        glClearColor(0,0,0,255);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents(); 
    
        /* @TODO remove on release, otherwise it saves cpu power drastically */
        Sleep(16);
    }


    glfwTerminate(); 
    return 0;
}
