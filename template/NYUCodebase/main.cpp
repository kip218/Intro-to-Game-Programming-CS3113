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

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
glm::mat4 modelMatrix = glm::mat4(1.0);
glm::mat4 viewMatrix = glm::mat4(1.0);
glm::mat4 projectionMatrix = glm::mat4(1.0);
ShaderProgram program;
bool gameDone = false;
float lastFrameTicks = 0.0;
const Uint8 *keys = SDL_GetKeyboardState(NULL);

class Object {
public:
    Object(float pixelX, float pixelY, float velocityX, float velocityY, float pixelWidth,
           float pixelHeight, float colorR, float colorG, float colorB)
    : pixelX(pixelX), pixelY(pixelY), velocityX(velocityX), velocityY(velocityY), pixelWidth(pixelWidth),
    pixelHeight(pixelHeight), colorR(colorR), colorG(colorG), colorB(colorB) {}
    
    void draw(ShaderProgram &p) {
        // Transforming matrix
        float unitX = ((pixelX / 960) * 2.666) - 1.333;
        float unitY = (((720 - pixelY) / 720) * 2.0) - 1.0;
        float width = (pixelWidth / 960) * 2.666;
        float height = (pixelHeight / 720) * 2.0;
        glm::mat4 objectModelMatrix = glm::translate(modelMatrix, glm::vec3(unitX, unitY, 0.0));
        objectModelMatrix = glm::scale(objectModelMatrix, glm::vec3(width, height, 1.0));
        p.SetModelMatrix(objectModelMatrix);
        
        // Draw
        float vertices[] = {0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5};
        glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(p.positionAttribute);
        p.SetColor(colorR, colorG, colorB, 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    float pixelX;
    float pixelY;
    float velocityX;
    float velocityY;
    float pixelWidth;
    float pixelHeight;
    float colorR;
    float colorG;
    float colorB;
};

// Helper functions
void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    glViewport(0, 0, 960, 720);
    projectionMatrix = glm::ortho(-1.333, 1.333, -1.0, 1.0, -1.0, 1.0);
    program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    glUseProgram(program.programID);
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
}

void ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            gameDone = true;
        }
    }
}

void Update() {
    // Update ticks
    float ticks = SDL_GetTicks()/1000.0;
    float elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;
    
}

void Render() {
    // Clearing screen
    glClearColor(0.05, 0.08, 0.12, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw

    
    // glDisables
    glDisableVertexAttribArray(program.positionAttribute);
    // Display
    SDL_GL_SwapWindow(displayWindow);
}

// Main
int main(int argc, char *argv[]) {
    Setup();
#ifdef _WINDOWS
    glewInit();
#endif
    while (!gameDone) {
        ProcessEvents();
        Update();
        Render();
    }
    SDL_Quit();
    return 0;
}
