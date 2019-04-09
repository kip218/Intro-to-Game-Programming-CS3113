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
enum EnemyState { MOVE_LEFT, MOVE_RIGHT, MOVE_DOWN };
EnemyState state;
#define MAX_BULLETS 30


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
    glm::vec3 position;

    Object(float x, float y)
        : position(x, y, 0) {}
};

class GameObject : public Object {
public:
    glm::vec3 velocity;
    glm::vec3 size;

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
    bool checkCollision(GameObject obj) {
        if (abs(position[0] - obj.position[0]) < (size[0] + obj.size[0]) / 2 &&
            abs(position[1] - obj.position[1]) < (size[1] + obj.size[1]) / 2) {
            return true;
        }
        return false;
    }
};

class Player : public GameObject {
public:
    Player() : GameObject(480, 660, 70, 70) {}
    
    float texCoords[12] = {310.0f/1024.0f, 941.0f/1024.0f, 211.0f/1024.0f, 1016.0f/1024.0f, 310.0f/1024.0f, 1016.0f/1024.0f, 211.0f/1024.0f, 941.0f/1024.0f, 211.0f/1024.0f, 1016.0f/1024.0f, 310.0f/1024.0f, 941.0f/1024.0f};
    
    void reset() {
        position[0] = 480;
        position[1] = 660;
        size[0] = 70;
        size[1] = 70;
    }
};

class Bullet : public GameObject {
public:
    Bullet() : GameObject(-3000, 0, 10, 40) {}
    
    float texCoords[12] = {865.0f/1024.0f, 983.0f/1024.0f, 856.0f/1024.0f, 1020.0f/1024.0f, 865.0f/1024.0f, 1020.0f/1024.0f, 856.0f/1024.0f, 983.0f/1024.0f, 856.0f/1024.0f, 1020.0f/1024.0f, 865.0f/1024.0f, 983.0f/1024.0f};
    
    void reset() {
        position[0] = -3000;
        position[1] = 0;
        velocity[0] = 0;
        velocity[1] = 0;
    }
};

class Enemy : public GameObject {
public:
    Enemy(float x, float y, float width, float height) : GameObject(x, y, width, height) {}
    
    float texCoords[12] = {535.0f/1024.0f, 0.0f/1024.0f, 444.0f/1024.0f, 91.0f/1024.0f, 535.0f/1024.0f, 91.0f/1024.0f, 444.0f/1024.0f, 0.0f/1024.0f, 444.0f/1024.0f, 91.0f/1024.0f, 535.0f/1024.0f, 0.0f/1024.0f};
};

class TextBox : public Object {
public:
    std::string text;
    float fontSize;

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
};

class TitleScreen {
public:
    TextBox title;
    TextBox playButton;
    bool goToGameLevel;

    TitleScreen()
        : title(480, 250, 0.2, "Space Invaders"), playButton(480, 450, 0.2, "Play"), goToGameLevel(false) {};
    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                gameDone = true;
            }
        }
        int x;
        int y;
        SDL_GetMouseState(&x, &y);
        if (playButton.position[0] - 100 < x && playButton.position[0] + 60 > x &&
            playButton.position[1] - 60 < y && playButton.position[1] > y) {
            // Enlarge when mouse hovers
            playButton.fontSize = 0.23;
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                goToGameLevel = true;
            }
        } else {
            playButton.fontSize = 0.2;
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
};

class GameLevel {
public:
    int bulletIndex = 0;
    Bullet bullets[MAX_BULLETS];
    int cooldown = 15;
    std::vector<Enemy> enemies;
    float maxMoveDownTime = 0.5;
    bool goLeft = true;
    Player player;

    GameLevel() {
        for (int i = 0; i < MAX_BULLETS; ++i) {
            bullets[i] = Bullet();
        }
        for (int i = 0; i < 36; ++i) {
            enemies.push_back(Enemy(90 * (i % 6 + 1) + 160, 70 * (i / 6 + 1) - 20, 60, 60));
            enemies[i].velocity[0] = -150;
        }
    }
    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                gameDone = true;
            }
            // Bullets
            if (keys[SDL_SCANCODE_SPACE] && cooldown <= 0) {
                shootBullet();
                cooldown = 15;
            }
            // Player movement
            if (keys[SDL_SCANCODE_RIGHT]) {
                player.velocity[0] = 400;
            } else if (keys[SDL_SCANCODE_LEFT]) {
                player.velocity[0] = -400;
            } else {
                player.velocity[0] = 0;
            }
        }
    }
    void update(float elapsed) {
        // Check game over
        for (int i = 0; i < enemies.size(); ++i) {
            if (enemies[i].checkCollision(player) ||
                enemies[i].position[1] + enemies[i].size[1] >= 720) {
                std::cout << "Game Over" << std::endl;
                reset();
                mode = TITLE_SCREEN;
            }
        }
        // Player
        if (0 < (player.position[0] - player.size[0] + player.velocity[0] * elapsed) && (player.position[0] + player.size[0] + player.velocity[0] * elapsed) < 960) {
            player.position[0] += player.velocity[0] * elapsed;
        }
        // Bullets
        for (int i = 0; i < MAX_BULLETS; ++i) {
            if (-200 < bullets[i].position[0] && bullets[i].position[0] < 1160 &&
                -200 < bullets[i].position[1] && bullets[i].position[1] < 920) {
                bullets[i].position[1] += bullets[i].velocity[1] * elapsed;
                for (int j = 0; j < enemies.size(); ++j) {
                    if (bullets[i].checkCollision(enemies[j])) {
                        enemies.erase(enemies.begin() + j);
                        bullets[i].reset();
                    }
                }
            } else {
                bullets[i].reset();
            }
        }
        // Enemies
        switch (state) {
            case MOVE_LEFT:
                for (int i = 0; i < enemies.size(); ++i) {
                    enemies[i].velocity[0] = -140;
                    enemies[i].velocity[1] = 0;
                    if (enemies[i].position[0] - enemies[i].size[0] <= 0) {
                        goLeft = false;
                        state = MOVE_DOWN;
                    }
                }
                break;
            case MOVE_RIGHT:
                for (int i = 0; i < enemies.size(); ++i) {
                    enemies[i].velocity[0] = 140;
                    enemies[i].velocity[1] = 0;
                    if (enemies[i].position[0] + enemies[i].size[0] >= 960) {
                        goLeft = true;
                        state = MOVE_DOWN;
                    }
                }
                break;
            case MOVE_DOWN:
                maxMoveDownTime = maxMoveDownTime - elapsed;
                for (int i = 0; i < enemies.size(); ++i) {
                    enemies[i].velocity[0] = 0;
                    enemies[i].velocity[1] = 100;
                    if (maxMoveDownTime < 0) {
                        if (goLeft) {
                            state = MOVE_LEFT;
                        } else {
                            state = MOVE_RIGHT;
                        }
                        maxMoveDownTime = 0.5;
                    }
                }
                break;
        }
        for (int i = 0; i < enemies.size(); ++i) {
            if (enemies[i].position[0] - enemies[i].size[0] <= 0) {
                for (int j = 0; j < enemies.size(); ++j) {
                    enemies[j].velocity[0] = 150;
                    enemies[j].velocity[1] = 50;
                }
                break;
            } else if (enemies[i].position[0] + enemies[i].size[0] >= 960) {
                for (int j = 0; j < enemies.size(); ++j) {
                    enemies[j].velocity[0] = -150;
                    enemies[j].velocity[1] = 50;
                }
                break;
            }
        }
        for (int i = 0; i < enemies.size(); ++i) {
            enemies[i].position[0] += enemies[i].velocity[0] * elapsed;
            enemies[i].position[1] += enemies[i].velocity[1] * elapsed;
        }
        if (cooldown > 0) { cooldown -= elapsed; }  // Cooldown working weirdly
        if (enemies.empty()) {
            std::cout << "You Win!" << std::endl;
            reset();
            mode = TITLE_SCREEN;
        }
    }
    void render() {
        player.draw(texProgram, player.texCoords);
        for (int i = 0; i < MAX_BULLETS; ++i) {
            bullets[i].draw(texProgram, bullets[i].texCoords);
        }
        for (int i = 0; i < enemies.size(); ++i) {
            enemies[i].draw(texProgram, enemies[i].texCoords);
        }
    }
    void shootBullet() {
        bullets[bulletIndex].position[0] = player.position[0];
        bullets[bulletIndex].position[1] = player.position[1];
        bullets[bulletIndex].velocity[1] = -800;
        bulletIndex++;
        if (bulletIndex > MAX_BULLETS-1) {
            bulletIndex = 0;
        }
    }
    void reset() {
        enemies.clear();
        for (int i = 0; i < 36; ++i) {
            enemies.push_back(Enemy(90 * (i % 6 + 1) + 160, 70 * (i / 6 + 1) - 20, 60, 60));
            enemies[i].velocity[0] = -150;
        }
        for (int i = 0; i < MAX_BULLETS; ++i) {
            bullets[i].reset();
        }
        player.reset();
    }
};


// Main function prototypes
void Setup();
void ProcessEvents();
void Update(float elapsed);
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
        // Update ticks
        float ticks = SDL_GetTicks()/1000.0;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        ProcessEvents();
        Update(elapsed);
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

void Update(float elapsed) {
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
