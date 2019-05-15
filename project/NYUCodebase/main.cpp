#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
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
float lastFrameTicks = 0.0;
float accumulator = 0.0;
glm::vec3 gravity(0.0, -2.0, 0.0);
const Uint8 *keys = SDL_GetKeyboardState(NULL);
enum GameMode { MENU, LEVEL };
GLuint font;
// sounds
Mix_Music *music;
Mix_Chunk *jump;
Mix_Chunk *landing;

/*
 Functions
*/
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(image);
    return retTexture;
}

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
    float retVal = dstMin + ((value - srcMin)/(srcMax-srcMin) * (dstMax-dstMin));
    if(retVal < dstMin) {
        retVal = dstMin;
    }
    if(retVal > dstMax) {
        retVal = dstMax;
    }
    return retVal;
}


/*
 Classes
*/
class TextBox{
public:
    TextBox(float x, float y, float fontSize, std::string text)
    : position(x, y, 0), fontSize(fontSize), text(text) {}
    void draw(ShaderProgram &p) {
        glm::mat4 textBoxModelMatrix = glm::translate(modelMatrix, position);
        p.SetModelMatrix(textBoxModelMatrix);
        DrawText(p, font, text, fontSize, 0);
    }
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
        glDrawArrays(GL_TRIANGLES, 0, 6 * (int)text.size());
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }
    
    glm::vec3 position;
    std::string text;
    float fontSize;
};

class Entity {
public:
    Entity(float x, float y, float width, float height, float r, float g, float b, float a)
    : position(x, y, 0), size(width, height, 1), color(r, g, b, a), angle(0), scaleX(1), scaleY(1) {}
    void draw(ShaderProgram &p) const {
        glm::mat4 entityModelMatrix = glm::translate(modelMatrix, position);
        entityModelMatrix = glm::rotate(entityModelMatrix, angle * (3.1415926f / 180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        entityModelMatrix = glm::scale(glm::scale(entityModelMatrix, size), glm::vec3(scaleX, scaleY, 1.0f));
        p.SetModelMatrix(entityModelMatrix);
        float vertices[] = {-0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5};
        glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(p.positionAttribute);
        p.SetColor(color.r, color.g, color.b, color.a);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(p.positionAttribute);
    }
    
    glm::vec3 position;
    glm::vec3 size;
    glm::vec4 color;
    float angle;
    float scaleX;
    float scaleY;
};

class Player : public Entity {
public:
    Player(float x, float y, float width, float height, float r, float g, float b, float a, float velocityX, float velocityY)
        : Entity(x, y, width, height, r, g, b, a), velocity(velocityX, velocityY, 0),
          acceleration(0, 0, 0), angleVelocity(0) {}
    void update(float elapsed, const std::vector<Entity> &platforms, const std::vector<Entity> &obstacles, Player &player) {
        velocity.y += gravity.y * elapsed;
        velocity += acceleration * elapsed;
        // collision bools
        collidedTop = false;
        collidedBottom = false;
        collidedLeft = false;
        collidedRight = false;
        // check x collision
        position.x += velocity.x * elapsed;
        adjustCollisionsX(platforms);
        adjustCollisionsX(obstacles);
        if (isColliding(player)) {
            float penetration = fabs(fabs(position.x - player.position.x) - fabs(size.x/2 + player.size.x/2));
            if (position.x < player.position.x) {
                position.x -= (penetration + 0.0001);
                collidedRight = true;
            } else {
                position.x += (penetration + 0.0001);
                collidedLeft = true;
            }
            velocity.x = 0;
        }
        // check y collision
        position.y += velocity.y * elapsed;
        adjustCollisionsY(platforms);
        adjustCollisionsY(obstacles);
        if (isColliding(player)) {
            float penetration = fabs(fabs(position.y - player.position.y) - fabs(size.y/2 + player.size.y/2));
            if (position.y < player.position.y) {
                position.y -= (penetration + 0.0001);
                collidedTop = true;
            } else {
                position.y += (penetration + 0.0001);
                collidedBottom = true;
            }
            velocity.y = 0;
        }
        // rotation
        angle += angleVelocity * elapsed;
        if (collidedBottom) {
            angle = 0;
            angleVelocity = 0;
        } else if (velocity.x < 0) {
            angleVelocity += 15;
        } else if (velocity.x > 0){
            angleVelocity -= 15;
        }
        if (collidedBottom && !onGround) { Mix_PlayChannel(-1, landing, 0); onGround = true; }
        if (!collidedBottom) { onGround = false; }
        scaleY = mapValue(fabs(velocity.y), 0.0, 7.0, 1.0, 2.0);
        scaleX = mapValue(fabs(velocity.y), 7.0, 0.0, 0.4, 1.0);
    }
    void process(const Uint8 *keys) {
        if (keys[upKey] && collidedBottom) {
            velocity.y = 1.25;
            Mix_PlayChannel(-1, jump, 0);
        }
        if (keys[rightKey]) {
            velocity.x = 0.5;
        } else if (keys[leftKey]) {
            velocity.x = -0.5;
        } else {
            velocity.x = 0;
        }
    }
    bool isColliding(Entity entity) const {
        if (fabs(position.x - entity.position.x) < (size.x/2 + entity.size.x/2) &&
            fabs(position.y - entity.position.y) < (size.y/2 + entity.size.y/2)) {
            return true;
        }
        return false;
    }
    void adjustCollisionsX(const std::vector<Entity> &solids) {
        for (const Entity &solid : solids) {
            if (isColliding(solid)) {
                float penetration = fabs(fabs(position.x - solid.position.x) - fabs(size.x/2 + solid.size.x/2));
                if (position.x < solid.position.x) {
                    position.x -= (penetration + 0.0001);
                    collidedRight = true;
                } else {
                    position.x += (penetration + 0.0001);
                    collidedLeft = true;
                }
                velocity.x = 0;
            }
        }
    }
    void adjustCollisionsY(const std::vector<Entity> &solids) {
        for (const Entity &solid : solids) {
            if (isColliding(solid)) {
                float penetration = fabs(fabs(position.y - solid.position.y) - fabs(size.y/2 + solid.size.y/2));
                if (position.y < solid.position.y) {
                    position.y -= (penetration + 0.0001);
                    collidedTop = true;
                } else {
                    position.y += (penetration + 0.0001);
                    collidedBottom = true;
                }
                velocity.y = 0;
            }
        }
    }

    glm::vec3 velocity;
    glm::vec3 acceleration;
    float angleVelocity;
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
    bool onGround;
    int upKey;
    int rightKey;
    int leftKey;
};

class Camera {
public:
    Camera(float x, float y, float width, float height, float velocityX, float velocityY)
        : position(x, y, 0), size(width, height, 0), velocity(velocityX, velocityY, 0) {}
    void setViewMatrix(ShaderProgram &p) const {
        glm::vec3 scale(size.x/3.2, size.y/2.0, 0);
        p.SetViewMatrix(glm::scale(glm::translate(viewMatrix, -position), scale));
    }
    void move(float elapsed) {
        position += velocity * elapsed;
    }
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 velocity;
};

class Level {
public:
    Level()
        : player1(0, 1.0, 0.099, 0.099, 0.75, 0.33, 0.33, 0.8, 0.3, 0),
          player2(0, 1.0, 0.099, 0.099, 0.33, 0.33, 0.75, 0.8, 0.3, 0),
          camera(0, 0, 3.2, 2.0, 0.3, 0),
          paused(false), escPressed(false), goToMenu(false), restart(false), gameOver(false) {
              // player key bindings
              player1.upKey = SDL_SCANCODE_UP;
              player1.rightKey = SDL_SCANCODE_RIGHT;
              player1.leftKey = SDL_SCANCODE_LEFT;
              player2.upKey = SDL_SCANCODE_W;
              player2.rightKey = SDL_SCANCODE_D;
              player2.leftKey = SDL_SCANCODE_A;
              // background panels
              background.push_back(Entity(-1.8, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
              background.push_back(Entity(-1.4, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
              background.push_back(Entity(-1.0, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
              background.push_back(Entity(-0.6, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
              background.push_back(Entity(-0.2, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
              background.push_back(Entity(0.2, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
              background.push_back(Entity(0.6, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
              background.push_back(Entity(1.0, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
              background.push_back(Entity(1.4, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
              background.push_back(Entity(1.8, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
              left_panel_i = 0;
              right_panel_i = background.size() - 1;
              // platforms
              platforms.push_back(Entity(0, -10.5, 6.4, 20, 0.30, 0.45, 0.45, 1));
              platforms.push_back(Entity(6.4, -10.5, 6.4, 20, 0.30, 0.45, 0.45, 1));
              left_platform_i = 0;
              right_platform_i = platforms.size() - 1;
              // obstacles
              for (int i=0; i < 45; i++) {
                  obstacles.push_back(Entity(-10, -0.45, 0.1, 0.1, 0.75, 0.75, 0.33, 1));
              }
    }
    void render(ShaderProgram &p, ShaderProgram &pTex) const {
        camera.setViewMatrix(p);
        for (const Entity &panel : background) {
            panel.draw(p);
        }
        player1.draw(p);
        player2.draw(p);
        for (const Entity &platform : platforms) {
            platform.draw(p);
        }
        for (const Entity &obstacle : obstacles) {
            obstacle.draw(p);
        }
        if (paused) {
            Entity shade(camera.position.x, camera.position.y, 3.2, 2.0, 0.2, 0.2, 0.2, 0.4);
            TextBox pause(-0.45, 0.4, 0.18, "Paused");
            TextBox resume(-0.504, 0.0, 0.05, "Press space to resume");
            TextBox restart(-0.418, -0.15, 0.05, "Press R to restart");
            TextBox quit(-0.66, -0.3, 0.05, "Press Q to quit to main menu");
            shade.draw(p);
            pause.draw(pTex);
            resume.draw(pTex);
            restart.draw(pTex);
            quit.draw(pTex);
        } else if (gameOver) {
            Entity shade(camera.position.x, camera.position.y, 3.2, 2.0, 0.2, 0.2, 0.2, 0.4);
            TextBox pause(-0.66, 0.4, 0.18, "Game Over");
            TextBox restart(-0.418, -0.10, 0.05, "Press R to restart");
            TextBox quit(-0.66, -0.25, 0.05, "Press Q to quit to main menu");
            shade.draw(p);
            pause.draw(pTex);
            restart.draw(pTex);
            quit.draw(pTex);
        }
    }
    void update(float elapsed, GameMode &mode) {
        if (goToMenu) { mode = MENU; goToMenu = false; reset(); return; }
        if (restart) { restart = false; reset(); return; }
        if (!paused && !gameOver) {
            camera.move(elapsed);
            player1.update(elapsed, platforms, obstacles, player2);
            player2.update(elapsed, platforms, obstacles, player1);
            // consistent map generation
            generateMap(background, left_panel_i, right_panel_i);
            generateMap(platforms, left_platform_i, right_platform_i);
            // obstacle generation
            generateObstacles(obstacles);
        }
        if (player1.position.x + player1.size.x/2 < camera.position.x - camera.size.x/2) {
            gameOver = true;
        } else if (player2.position.x + player2.size.x/2 < camera.position.x - camera.size.x/2) {
            gameOver = true;
        }
    }
    void process(SDL_Event &event, const Uint8 *keys) {
        // pausing
        if (!gameOver) {
            if (keys[SDL_SCANCODE_ESCAPE] && !escPressed) { paused = !paused; escPressed = true; }
            if (!keys[SDL_SCANCODE_ESCAPE]) { escPressed = false; }
        }
        if (!paused && !gameOver) {
            player1.process(keys);
            player2.process(keys);
        } else if (paused) {
            if (keys[SDL_SCANCODE_SPACE]) {
                paused = false;
            } else if (keys[SDL_SCANCODE_Q]) {
                goToMenu = true;
            } else if (keys[SDL_SCANCODE_R]) {
                restart = true;
            }
        } else if (gameOver) {
            if (keys[SDL_SCANCODE_Q]) {
                goToMenu = true;
            } else if (keys[SDL_SCANCODE_R]) {
                restart = true;
            }
        }
    }
    void generateMap(std::vector<Entity> &vector, size_t &left_i, size_t &right_i) {
        if (vector[left_i].position.x + vector[right_i].size.x/2
            < camera.position.x - camera.size.x/2) {
            vector[left_i].position.x = vector[right_i].position.x + vector[left_i].size.x;
            left_i++;
            right_i++;
            if (left_i >= vector.size()) { left_i = 0; }
            if (right_i >= vector.size()) { right_i = 0; }
        }
    }
    void generateObstacles(std::vector<Entity> &vector) {
        for (int i=0; i < vector.size(); i++) {
            if (vector[i].position.x + vector[i].size.x/2
                < camera.position.x - camera.size.x/2) {
                float x = camera.position.x + camera.size.x/2 + (rand() % 32) * 0.1;
                x = floor(x * 10) / 10 + 0.15;
                float y = -0.45 + (rand() % 12) * 0.101;
                y = floor(y * 10) / 10 + 0.05;
                vector[i].position.x = x;
                vector[i].position.y = y;
            }
        }
    }
    void reset() {
        player1 = Player(0, 1.0, 0.099, 0.099, 0.75, 0.33, 0.33, 0.8, 0.3, 0);
        player2 = Player(0, 1.0, 0.099, 0.099, 0.33, 0.33, 0.75, 0.8, 0.3, 0);
        camera = Camera(0, 0, 3.2, 2.0, 0.3, 0);
        paused = false;
        escPressed = false;
        goToMenu = false;
        gameOver = false;
        // player key bindings
        player1.upKey = SDL_SCANCODE_UP;
        player1.rightKey = SDL_SCANCODE_RIGHT;
        player1.leftKey = SDL_SCANCODE_LEFT;
        player2.upKey = SDL_SCANCODE_W;
        player2.rightKey = SDL_SCANCODE_D;
        player2.leftKey = SDL_SCANCODE_A;
        // background panels
        background.clear();
        background.push_back(Entity(-1.8, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
        background.push_back(Entity(-1.4, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
        background.push_back(Entity(-1.0, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
        background.push_back(Entity(-0.6, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
        background.push_back(Entity(-0.2, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
        background.push_back(Entity(0.2, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
        background.push_back(Entity(0.6, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
        background.push_back(Entity(1.0, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
        background.push_back(Entity(1.4, 0, 0.4, 2.0, 0.26, 0.26, 0.26, 1));
        background.push_back(Entity(1.8, 0, 0.4, 2.0, 0.4, 0.4, 0.4, 1));
        left_panel_i = 0;
        right_panel_i = background.size() - 1;
        // platforms
        platforms.clear();
        platforms.push_back(Entity(0, -10.5, 6.4, 20, 0.30, 0.45, 0.45, 1));
        platforms.push_back(Entity(6.4, -10.5, 6.4, 20, 0.30, 0.45, 0.45, 1));
        left_platform_i = 0;
        right_platform_i = platforms.size() - 1;
        // obstacles
        obstacles.clear();
        for (int i=0; i < 50; i++) {
            obstacles.push_back(Entity(-10, -0.45, 0.1, 0.1, 0.75, 0.75, 0.33, 1));
        }
    }

    Player player1;
    Player player2;
    std::vector<Entity> background;
    std::vector<Entity> platforms;
    std::vector<Entity> obstacles;
    Camera camera;
    size_t left_panel_i;
    size_t right_panel_i;
    size_t left_platform_i;
    size_t right_platform_i;
    bool paused;
    bool escPressed;
    bool gameOver;
    bool goToMenu;
    bool restart;
};

class Menu {
public:
    Menu()
        : title(-0.99, 0.4, 0.22, "Block Dash"),
          play(-0.93, -0.2, 0.1, "Press space to play"),
          quit(-1.57, 0.97, 0.03, "Press escape to quit"),
          goToGameLevel(false) {}
    void process(SDL_Event &event, const Uint8 *keys) {
        if (keys[SDL_SCANCODE_ESCAPE]) {
            gameDone = true;
        }
        if (keys[SDL_SCANCODE_SPACE]) {
            goToGameLevel = true;
        }
    }
    void update(float elapsed, GameMode &mode) {
        if (goToGameLevel) {
            mode = LEVEL;
            goToGameLevel = false;
        }
    }
    void render(ShaderProgram &p, ShaderProgram &pTex) {
        title.draw(pTex);
        play.draw(pTex);
        quit.draw(pTex);
    }
    
    TextBox title;
    TextBox play;
    TextBox quit;
    bool goToGameLevel;
};

Level level;
Menu menu;
GameMode mode;


/*
Main helper functions
*/
void setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    glViewport(0, 0, 1280, 800);
    projectionMatrix = glm::ortho(-1.6, 1.6, -1.0, 1.0, -1.0, 1.0);
    // program
    program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    programTex.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glUseProgram(program.programID);
    program.SetViewMatrix(viewMatrix);
    program.SetProjectionMatrix(projectionMatrix);
    // texture program
    programTex.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glUseProgram(programTex.programID);
    programTex.SetProjectionMatrix(projectionMatrix);
    programTex.SetViewMatrix(viewMatrix);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Textures
    font = LoadTexture(RESOURCE_FOLDER"font.png");
    // Sounds
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    music = Mix_LoadMUS(RESOURCE_FOLDER"Chiptronical.mp3");
    jump = Mix_LoadWAV(RESOURCE_FOLDER"jump.wav");
    landing = Mix_LoadWAV(RESOURCE_FOLDER"landing.wav");
    Mix_VolumeMusic(60);
    Mix_VolumeChunk(jump, 100);
    Mix_VolumeChunk(landing, 100);
    Mix_PlayMusic(music, -1);
}

void process() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            gameDone = true;
        }
    }
    switch (mode) {
        case MENU:
            menu.process(event, keys);
            break;
        case LEVEL:
            level.process(event, keys);
            break;
    }
}

void update(float elapsed) {
    switch (mode) {
        case MENU:
            menu.update(elapsed, mode);
            break;
        case LEVEL:
            level.update(elapsed, mode);
            break;
    }
}

void render() {
    glClearColor(0.30, 0.45, 0.45, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    switch (mode) {
        case MENU:
            menu.render(program, programTex);
            break;
        case LEVEL:
            level.render(program, programTex);
            break;
    }
    SDL_GL_SwapWindow(displayWindow);
    glFlush();
}

int main() {
    setup();
#ifdef _WINDOWS
    glewInit();
#endif
    while (!gameDone) {
        float ticks = SDL_GetTicks()/1000.0;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        elapsed += accumulator;
        if (elapsed < FIXED_TIMESTEP) {
            accumulator = elapsed;
            continue;
        }
        process();
        while (elapsed >= FIXED_TIMESTEP) {
            update(elapsed);
            elapsed -= FIXED_TIMESTEP;
        }
        render();
    }
    Mix_FreeMusic(music);
    Mix_FreeChunk(jump);
    Mix_FreeChunk(landing);
    SDL_Quit();
    return 0;
}
