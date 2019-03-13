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

// Although the base and derived classes below aren't really necessary for a game of pong, I thought I'd practice abstracting game objects for future assignments.
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

class MovingObject : public Object {
public:
    using Object::Object;
    
    bool checkCollision(Object obj, float elapsed) {
        if (abs(pixelX + velocityX * elapsed - obj.pixelX) < (pixelWidth + obj.pixelWidth) / 2 &&
            abs(pixelY + velocityY * elapsed - obj.pixelY) < (pixelHeight + obj.pixelHeight) / 2) {
            return true;
        }
        return false;
    }
    
    virtual void reset() = 0;
};

class Ball : public MovingObject {
public:
    using MovingObject::MovingObject;
    
    void reset() {
        pixelX = 480;
        pixelY = 360;
        velocityX = 0;
        velocityY = 0;
    }
};
class Player : public MovingObject {
public:
    using MovingObject::MovingObject;
    
    void reset() {
        pixelY = 360;
        velocityX = 0;
        velocityY = 0;
    }
};
class Wall : public Object {
public:
    using Object::Object;
};
class Goal : public Object {
public:
    using Object::Object;
};

// Initializing game objects
Player player1(0, 360, 0, 0, 60, 150, 0.87, 0.45, 0.05);
Player player2(960, 360, 0, 0, 60, 150, 0.44, 0.76, 0.87);
Ball ball(480, 360, -300, 0, 20, 20, 1.00, 0.90, 0.30);
Wall upperWall(480, 720, 0, 0, 960, 40, 0.55, 0.55, 0.55);
Wall lowerWall(480, 0, 0, 0, 960, 40, 0.55, 0.55, 0.55);
Goal leftGoal(-100, 360, 0, 0, 200, 800, 0.55, 0.55, 0.55);
Goal rightGoal(1160, 360, 0, 0, 200, 800, 0.55, 0.55, 0.55);

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
        
        // Player1 movement
        if (keys[SDL_SCANCODE_W]) {
            player1.velocityY = -300;
        } else if (keys[SDL_SCANCODE_S]) {
            player1.velocityY = 300;
        } else {
            player1.velocityY = 0;
        }
        
        // Player2 movement
        if (keys[SDL_SCANCODE_UP]) {
            player2.velocityY = -300;
        } else if (keys[SDL_SCANCODE_DOWN]) {
            player2.velocityY = 300;
        } else {
            player2.velocityY = 0;
        }
        
    }
}

void Update() {
    // Update ticks
    float ticks = SDL_GetTicks()/1000.0;
    float elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;
    
    if (!(player1.checkCollision(upperWall, elapsed)) && !(player1.checkCollision(lowerWall, elapsed))) {
        player1.pixelX += player1.velocityX * elapsed;
        player1.pixelY += player1.velocityY * elapsed;
    }
    if (!(player2.checkCollision(upperWall, elapsed)) && !(player2.checkCollision(lowerWall, elapsed))) {
        player2.pixelX += player2.velocityX * elapsed;
        player2.pixelY += player2.velocityY * elapsed;
    }
    if (ball.checkCollision(player1, elapsed)) {
        ball.velocityX *= -1.1;
        ball.velocityY = (player1.velocityY * 1.1 + ball.velocityY) / 2;
    } else if (ball.checkCollision(player2, elapsed)) {
        ball.velocityX *= -1.1;
        ball.velocityY = (player2.velocityY * 1.1 + ball.velocityY) / 2;
    } else if (ball.checkCollision(leftGoal, elapsed)) {
        std::cout << "Player2 wins!";
        ball.reset();
        ball.velocityX = -300;
        player1.reset();
        player2.reset();
    } else if (ball.checkCollision(rightGoal, elapsed)) {
        std::cout << "Player1 wins!";
        ball.reset();
        ball.velocityX = 300;
        player1.reset();
        player2.reset();
    } else if (ball.checkCollision(upperWall, elapsed) || ball.checkCollision(lowerWall, elapsed)) {
        ball.velocityY *= -1;
    } else {
        ball.pixelX += ball.velocityX * elapsed;
        ball.pixelY += ball.velocityY * elapsed;
    }
}

void Render() {
    // Clearing screen
    glClearColor(0.05, 0.08, 0.12, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw
    player1.draw(program);
    player2.draw(program);
    ball.draw(program);
    upperWall.draw(program);
    lowerWall.draw(program);
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
