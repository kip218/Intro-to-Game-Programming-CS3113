#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "FlareMap.h"
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define FIXED_TIMESTEP 0.0166666

SDL_Window* displayWindow;
glm::mat4 modelMatrix(1.0);
glm::mat4 viewMatrix(1.0);
glm::mat4 projectionMatrix(1.0);
ShaderProgram program;
ShaderProgram programTex;
SDL_Event event;
bool gameDone = false;



/*
Main helper functions
*/


void setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
//    SDL_GL_MakeCurrent(displayWindow, context);
    glViewport(0, 0, 1280, 800);
    projectionMatrix = glm::ortho(-1.6, 1.6, -1.0, 1.0, -1.0, 1.0);
    program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    programTex.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glUseProgram(program.programID);
    program.SetViewMatrix(viewMatrix);
    program.SetProjectionMatrix(projectionMatrix);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            gameDone = true;
        }
    }
    
}

void update() {
    
}

void render() {
    
}






int main() {
    setup();
}
