/* glad + glfw */
#include "gl33.h"
#include "glfw3.h"

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
    
    while (!glfwWindowShouldClose(window)) {
        glfwSetTime(0.0);

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
