#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>

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
ShaderProgram texProgram;
bool gameDone = false;
float lastFrameTicks = 0.0;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
GLuint font;
GLuint spriteSheet;
enum GameMode { TITLE_SCREEN, GAME_LEVEL };
GameMode mode;


// Helper functions
void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
    float character_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for (int i=0; i < text.size(); ++i) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x + character_size, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x, texture_y + character_size,
        });
    }
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6 * text.size());
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if (image == NULL) {
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


// Game classes
class Object {
public:
    Object(float x, float y)
        : position(x, y, 0) {}
    glm::vec3 position;
};

class GameObject : public Object {
public:
    GameObject(float x, float y, float width, float height)
    : Object(x, y), size(width, height, 0) {}
    void draw(ShaderProgram &p, float texCoords[12]) {
        // Transforming matrix
        float glX = ((position[0] / 960) * 2.666) - 1.333;
        float glY = (((720 - position[1]) / 720) * 2.0) - 1.0;
        float glWidth = (size[0] / 960) * 2.666;
        float glHeight = (size[1] / 720) * 2.0;
        glm::mat4 objectModelMatrix = glm::translate(modelMatrix, glm::vec3(glX, glY, 0.0));
        objectModelMatrix = glm::scale(objectModelMatrix, glm::vec3(glWidth, glHeight, 1.0));
        p.SetModelMatrix(objectModelMatrix);
        // Draw
        float vertices[] = {0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5};
        glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(p.positionAttribute);
        glEnableVertexAttribArray(p.texCoordAttribute);
        glBindTexture(GL_TEXTURE_2D, spriteSheet);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(p.positionAttribute);
        glDisableVertexAttribArray(p.texCoordAttribute);
    }
    glm::vec3 velocity;
    glm::vec3 size;
};

class Player : public GameObject {
public:
    Player(float x, float y, float width, float height) : GameObject(x, y, width, height) {}
    
    float texCoords[12] = {310.0f/1024.0f, 941.0f/1024.0f, 211.0f/1024.0f, 1016.0f/1024.0f, 310.0f/1024.0f, 1016.0f/1024.0f, 211.0f/1024.0f, 941.0f/1024.0f, 211.0f/1024.0f, 1016.0f/1024.0f, 310.0f/1024.0f, 941.0f/1024.0f};
};

class TextBox : public Object {
public:
    TextBox(float x, float y, float fontSize, std::string text)
        : Object(x, y), fontSize(fontSize), text(text) {};
    void draw(ShaderProgram &p) {
        // Transforming matrix
        float glX = ((position[0] / 960) * 2.666) - 1.333 - (text.size() * fontSize)/4;
        float glY = (((720 - position[1]) / 720) * 2.0) - 1.0 + fontSize/2;
        glm::mat4 textBoxModelMatrix = glm::translate(modelMatrix, glm::vec3(glX, glY, 0.0));
        p.SetModelMatrix(textBoxModelMatrix);
        // Draw
        DrawText(p, font, text, fontSize, -0.1);
    }
    std::string text;
    float fontSize;
};

class TitleScreen {
public:
    TitleScreen()
        : title(480, 250, 0.2, "Space Invaders"), playButton(480, 450, 0.2, "Play"), goToGameLevel(false) {};
    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                gameDone = true;
            }
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (playButton.position[0] - 100 < event.button.x &&
                    playButton.position[0] + 60 > event.button.x &&
                    playButton.position[1] - 60 < event.button.y &&
                    playButton.position[1] > event.button.y) {
                        goToGameLevel = true;
                }
            }
        }
    }
    void update() {
        if (goToGameLevel) {
            mode = GAME_LEVEL;
            goToGameLevel = false;
        }
    }
    void render() {
        title.draw(texProgram);
        playButton.draw(texProgram);
    }
    TextBox title;
    TextBox playButton;
    bool goToGameLevel;
};

class GameLevel {
public:
    GameLevel()
        : player(480, 660, 80, 80) {}
    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                gameDone = true;
            }
            // Player movement
            if (keys[SDL_SCANCODE_RIGHT]) {
                player.velocity[0] = 300;
            } else if (keys[SDL_SCANCODE_LEFT]) {
                player.velocity[0] = -300;
            } else {
                player.velocity[0] = 0;
            }
        }
    }
    void update(float elapsed) {
        if (0 < player.position[0] - player.size[0] && player.position[0] + player.size[0] < 960) {
            
            player.position[0] += player.velocity[0] * elapsed;
        }
    }
    void render() {
        player.draw(texProgram, player.texCoords);
    }
    Player player;
};


// Main function prototypes
void Setup();
void ProcessEvents();
void Update();
void Render();

// In-game global variables
TitleScreen titleScreen;
GameLevel gameLevel;

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


// Main functions
void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    glViewport(0, 0, 960, 720);
    projectionMatrix = glm::ortho(-1.333, 1.333, -1.0, 1.0, -1.0, 1.0);
    // Normal program
    program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    glUseProgram(program.programID);
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    // Texture program
    texProgram.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(texProgram.programID);
    texProgram.SetProjectionMatrix(projectionMatrix);
    texProgram.SetViewMatrix(viewMatrix);
    // Textures
    font = LoadTexture(RESOURCE_FOLDER"assets/font.png");
    spriteSheet = LoadTexture(RESOURCE_FOLDER"assets/spritesheet.png");
}

void ProcessEvents() {
    switch (mode) {
        case TITLE_SCREEN:
            titleScreen.processEvents();
            break;
        case GAME_LEVEL:
            gameLevel.processEvents();
            break;
    }
}

void Update() {
    // Update ticks
    float ticks = SDL_GetTicks()/1000.0;
    float elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;
    switch (mode) {
        case TITLE_SCREEN:
            titleScreen.update();
            break;
        case GAME_LEVEL:
            gameLevel.update(elapsed);
            break;
    }
}

void Render() {
    // Clearing screen
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    switch (mode) {
        case TITLE_SCREEN:
            titleScreen.render();
            break;
        case GAME_LEVEL:
            gameLevel.render();
            break;
    }
    // Display
    SDL_GL_SwapWindow(displayWindow);
}
