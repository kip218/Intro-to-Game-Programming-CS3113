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

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
    // Setup & matrices
    glViewport(0, 0, 640, 360);
    // Setting default matrix
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    
    // Setting ground matrix
    glm::mat4 modelMatrixGround = glm::translate(modelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
    modelMatrixGround = glm::scale(modelMatrixGround, glm::vec3(5.0f, 1.0f, 1.0f));
    
    // Setting alien matrices
    glm::mat4 modelMatrixAlien1 = glm::translate(modelMatrix, glm::vec3(-1.5f, -0.22f, 0.0f));
    modelMatrixAlien1 = glm::scale(modelMatrixAlien1, glm::vec3(0.5f, 0.75f, 1.0f));
    glm::mat4 modelMatrixAlien2 = glm::translate(modelMatrix, glm::vec3(-0.75f, -0.22f, 0.0f));
    modelMatrixAlien2 = glm::scale(modelMatrixAlien2, glm::vec3(0.5f, 0.75f, 1.0f));
    glm::mat4 modelMatrixAlien3 = glm::translate(modelMatrix, glm::vec3(0.0f, -0.22f, 0.0f));
    modelMatrixAlien3 = glm::scale(modelMatrixAlien3, glm::vec3(0.5f, 0.75f, 1.0f));
    glm::mat4 modelMatrixAlien4 = glm::translate(modelMatrix, glm::vec3(0.75f, -0.22f, 0.0f));
    modelMatrixAlien4 = glm::scale(modelMatrixAlien4, glm::vec3(0.5f, 0.75f, 1.0f));
    glm::mat4 modelMatrixAlien5 = glm::translate(modelMatrix, glm::vec3(1.5f, -0.22f, 0.0f));
    modelMatrixAlien5 = glm::scale(modelMatrixAlien5, glm::vec3(0.5f, 0.75f, 1.0f));
    
    // Vertices & texture coordinates for texturing
    float vertices[] = {0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5};
    float texCoords[] = {1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0};

    // Normal shader
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    glUseProgram(program.programID);

    // Texture shader
    ShaderProgram program_textured;
    program_textured.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_textured.programID);
    
    // Texture assets
    GLuint alienTexture1 = LoadTexture(RESOURCE_FOLDER"assets/alienGreen.png");
    GLuint alienTexture2 = LoadTexture(RESOURCE_FOLDER"assets/alienPink.png");
    GLuint alienTexture3 = LoadTexture(RESOURCE_FOLDER"assets/alienYellow.png");
    GLuint alienTexture4 = LoadTexture(RESOURCE_FOLDER"assets/alienBlue.png");
    GLuint alienTexture5 = LoadTexture(RESOURCE_FOLDER"assets/alienBeige.png");
    

#ifdef _WINDOWS
    glewInit();
#endif

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
        // Clearing screen
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Setting up drawing ground
        program.SetModelMatrix(modelMatrixGround);
        program.SetProjectionMatrix(projectionMatrix);
        program.SetViewMatrix(viewMatrix);
        // Drawing ground
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        program.SetColor(0.5f, 0.3f, 0.3f, 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // glDisables
        glDisableVertexAttribArray(program.positionAttribute);
        
        // Setting up drawing aliens
        program_textured.SetProjectionMatrix(projectionMatrix);
        program_textured.SetViewMatrix(viewMatrix);
        glVertexAttribPointer(program_textured.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program_textured.positionAttribute);
        glVertexAttribPointer(program_textured.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program_textured.texCoordAttribute);
        // Drawing alien1
        program_textured.SetModelMatrix(modelMatrixAlien1);
        glBindTexture(GL_TEXTURE_2D, alienTexture1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Drawing alien2
        program_textured.SetModelMatrix(modelMatrixAlien2);
        glBindTexture(GL_TEXTURE_2D, alienTexture2);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Drawing alien3
        program_textured.SetModelMatrix(modelMatrixAlien3);
        glBindTexture(GL_TEXTURE_2D, alienTexture3);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Drawing alien4
        program_textured.SetModelMatrix(modelMatrixAlien4);
        glBindTexture(GL_TEXTURE_2D, alienTexture4);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Drawing alien5
        program_textured.SetModelMatrix(modelMatrixAlien5);
        glBindTexture(GL_TEXTURE_2D, alienTexture5);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // glDisables
        glDisableVertexAttribArray(program_textured.positionAttribute);
        glDisableVertexAttribArray(program_textured.texCoordAttribute);
        
        // Display
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
