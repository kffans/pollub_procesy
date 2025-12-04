#include "pch.hpp"

/* glad + glfw */
#include "gl33.h"
#include "glfw3.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

unsigned int           W_WIDTH       = 1024;
unsigned int           W_HEIGHT      = 512;
bool                   IS_PAUSED     = false;
double                 D_TIME        = 0;
std::array<GLfloat,16> PROJECTION;

#include "gl.hpp"
#include "lin.hpp"
#include "cam.hpp"
#include "font.hpp"
#include "text.hpp"
#define DIAL_DEBUG
#include "dial.hpp"

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
   
    font::CreateProgram();


    font::Font* font = font::Font_I(28);
    dial::State* state = dial::State_I("test");
    
    //double fontTime = 0.0;
    
    std::vector<text::Text*> texts = {};
    
    while (!glfwWindowShouldClose(window)) {
        D_TIME = glfwGetTime();
        glfwSetTime(0.0);

        if (!IS_PAUSED) {
            /* tick functions */
            dial::Dialogue_T(state);


            /* keyboard events */
            if (dial::IsCurrentStatus(state, dial::Status::WAIT_FOR_CONTINUATION)) {
                if (IsKeyInState(window, GLFW_KEY_ENTER, GLFW_PRESS) || IsKeyInState(window, GLFW_KEY_SPACE, GLFW_PRESS)) {
                    dial::Continuation(state);
                }
            }
            //  dial::IsCurrentStatus(state, dial::Status::WAIT_FOR_CHOICE)
            if (dial::IsCurrentStatus(state, dial::Status::WAIT_FOR_CHOICE)) {
                if (IsKeyInState(window, GLFW_KEY_A, GLFW_PRESS)) {
                    AccentDecrement(state);
                }
                if (IsKeyInState(window, GLFW_KEY_S, GLFW_PRESS)) {
                    AccentIncrement(state);
                }
                u32 choices_s = GetChoicesSize(state);
                for (u32 i = 0; i < choices_s; i++) {
                    /* @TODO checks here if the option is clicked with mouse */
                    if (IsKeyInState(window, GLFW_KEY_1 + i, GLFW_RELEASE)) {
                        bool isValid = dial::IsChoiceValid(state, i);
                        if (!isValid) { continue; }
                        dial::Choice(state, i);
                        break;
                    }
                }
            }
            if (state != nullptr && (state->status == dial::Status::WAIT_FOR_CONTINUATION || state->status == dial::Status::WAIT_FOR_CHOICE)) {
                if (IsKeyInState(window, GLFW_KEY_Q, GLFW_PRESS) || IsKeyInState(window, GLFW_KEY_SPACE, GLFW_PRESS)) {
                    dial::StateSave(state, 0);
                }
            }
            if (state != nullptr && (state->status == dial::Status::WAIT_FOR_CONTINUATION || state->status == dial::Status::WAIT_FOR_CHOICE)) {
                if (IsKeyInState(window, GLFW_KEY_W, GLFW_PRESS) || IsKeyInState(window, GLFW_KEY_SPACE, GLFW_PRESS)) {
                    system("cls");
                    dial::State_D(state);
                    state = dial::StateLoad("test", 0);
                }
            }
#ifdef DIAL_DEBUG
            if (IsKeyInState(window, GLFW_KEY_BACKSPACE, GLFW_PRESS)) { /* debug backtracking function; press the backspace */
                //system("cls");
                dial::LoadBacktrackState(state);
            }
            if (IsKeyInState(window, GLFW_KEY_D, GLFW_PRESS)) { /* debug */
                std::cout<<"\n--------------------+\n";
                if (state != nullptr) {
                    std::cout<<"Current position in file: "<<dial::GetCurrentTextFilePos(state->text, state->currentPos.text_i);
                }
                dial::ShowVars(state);
            }
            if (IsKeyInState(window, GLFW_KEY_R, GLFW_PRESS)) { /* reset */
                system("cls");
                dial::State_D(state);
                state = dial::State_I("test");
            }
#endif
        }
        elif (IS_PAUSED) {

        }

        if (IsKeyInState(window, GLFW_KEY_P, GLFW_PRESS)) { /* pause */
            IS_PAUSED = !IS_PAUSED;
        }
        if (IsKeyInState(window, GLFW_KEY_Q, GLFW_PRESS) || IsKeyInState(window, GLFW_KEY_ESCAPE, GLFW_PRESS)) { /* quit */
            break;
        }
        

        glClearColor(0,0,0,255);
        glClear(GL_COLOR_BUFFER_BIT);
        
        
        if (state != nullptr) {
            text::Text* actorNameText = text::Text_I(font, state->actor_n);
            actorNameText->transform = lin::Translate(200, 0) * actorNameText->transform;
            actorNameText->color = {0.6f, 0.0f, 0.0f, 1.0f}; 
            text::Draw(actorNameText);
            text::Text_D(actorNameText);

            int tY = 0;
            int tYmax = 0;
            for (u32 i = 0; i < state->textObjs.size(); i++) { texts.push_back(text::Text_I(font, state->textObjs[i].text)); }
            for (auto it = texts.begin(); it != texts.end(); ++it) { 
                tYmax += (*it)->lastCharPos.y - 28 - 10;
                (*it)->color = {1.0f, 1.0f, 1.0f, 1.0f}; 
            }
            for (auto it = texts.begin(); it != texts.end(); ++it) { 
                (*it)->transform = lin::Translate(-400, tY - tYmax -200) * (*it)->transform;
                tY += (*it)->lastCharPos.y - 28 - 10;
            }
            if (state != nullptr && state->status == dial::Status::WAIT_FOR_CHOICE) { /* @TODO change this if statement when adding dialogue settings */
                auto it = --texts.end();
                int choices_s = (int)GetChoicesSize(state);
                for (int choice_i = choices_s - 1; choice_i >= 0; choice_i--) {
                    if (HasOneUseChoiceRecurred(state, choice_i)) { 
                        (*it)->color = {0.6f, 0.6f, 0.6f, 1.0f}; 
                    }
                    it--;
                }
            }
            for (auto text : texts) {
                text::Draw(text);
                text::Text_D(text);
            }
            texts.clear();
        }
        

        glfwSwapBuffers(window);
        glfwPollEvents(); 
    
        /* @TODO remove on release, otherwise it saves cpu power drastically */
        Sleep(16);
    }

    /* deallocate */
    dial::State_D(state);
    font::Font_D(font);

    glfwTerminate(); 
    return 0;
}
