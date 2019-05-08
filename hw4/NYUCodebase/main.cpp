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

#define FIXED_TIMESTEP 0.0166666f
#define TILE_SIZE 0.1f

SDL_Window* displayWindow;
glm::mat4 modelMatrix = glm::mat4(1.0);
glm::mat4 viewMatrix = glm::mat4(1.0);
glm::mat4 projectionMatrix = glm::mat4(1.0);
ShaderProgram program;
SDL_Event event;
bool gameDone = false;
float lastFrameTicks = 0.0;
float accumulator = 0.0;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
FlareMap map;
GLuint mapSpriteID;
std::vector<float> tileMapVertices;
std::vector<float> tileMapTexCoords;
glm::vec3 gravity(0, -7.0, 0);
glm::vec3 friction(7.0, 0, 0);


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

class SheetSprite {
public:
    SheetSprite() {};
    SheetSprite(unsigned int textureID, int index, int spriteCountX, int spriteCountY)
        : textureID(textureID), index(index), spriteCountX(spriteCountX), spriteCountY(spriteCountY) {}
    void draw(ShaderProgram &program, glm::vec3 position, glm::vec3 size) const {
        glBindTexture(GL_TEXTURE_2D, textureID);
        float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
        float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
        float spriteWidth = 1.0/(float)spriteCountX;
        float spriteHeight = 1.0/(float)spriteCountY;
        float texCoords[] = {
            u, v+spriteHeight,
            u+spriteWidth, v,
            u, v,
            u+spriteWidth, v,
            u, v+spriteHeight,
            u+spriteWidth, v+spriteHeight
        };
        float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f,
            -0.5f, 0.5f, -0.5f};
        
        glm::mat4 spriteModelMatrix = glm::translate(modelMatrix, position);
        spriteModelMatrix = glm::scale(spriteModelMatrix, size);
        program.SetModelMatrix(spriteModelMatrix);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.positionAttribute);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }
    unsigned int textureID;
    int index;
    int spriteCountX;
    int spriteCountY;
};

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)(worldX / TILE_SIZE);
    *gridY = (int)(worldY / -TILE_SIZE);
}

class Entity {
public:
    Entity(float x, float y, float width, float height)
        : position(x, y, 0), velocity(0, 0, 0), acceleration(0, 0, 0), size(width, height, 1) {}
    void draw(ShaderProgram &p) const {
        sprite.draw(p, position, size);
    }
    void update(float elapsed) {
        // update movement
        // friction only when on ground
        if (collidedBottom) {
            velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
            velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);
        }
        velocity.y += gravity.y * elapsed;
        velocity += acceleration * elapsed;
        position += velocity * elapsed;
        
        collidedTop = false;
        collidedBottom = false;
        collidedLeft = false;
        collidedRight = false;
        int gridX;
        int gridY;

        // limit to tilemap
        worldToTileCoordinates(position.x, position.y, &gridX, &gridY);
        if (0 < gridX && gridX < 50 && 0 < gridY && gridY < 25) {
            // might need to add more collision points
            // check bottom collision
            worldToTileCoordinates(position.x, position.y - size.y/2, &gridX, &gridY);
            if (map.mapData[gridY][gridX]+1 == 4 || map.mapData[gridY][gridX]+1 == 7 || map.mapData[gridY][gridX]+1 == 18 || map.mapData[gridY][gridX]+1 == 34) {
                position.y += (-TILE_SIZE * gridY) - (position.y - size.y/2);
                velocity.y = 0;
                collidedBottom = true;
            }
            // check top collision
            worldToTileCoordinates(position.x, position.y + size.y/2, &gridX, &gridY);
            if (map.mapData[gridY][gridX]+1 == 4 || map.mapData[gridY][gridX]+1 == 7 || map.mapData[gridY][gridX]+1 == 18 || map.mapData[gridY][gridX]+1 == 34) {
                position.y += (-TILE_SIZE * gridY - TILE_SIZE) - (position.y + size.y/2);
                velocity.y = 0;
                collidedTop = true;
            }
            // check left collision
            worldToTileCoordinates(position.x - size.x/2, position.y, &gridX, &gridY);
            if (map.mapData[gridY][gridX]+1 == 4 || map.mapData[gridY][gridX]+1 == 7 || map.mapData[gridY][gridX]+1 == 18 || map.mapData[gridY][gridX]+1 == 34) {
                position.x += (TILE_SIZE * gridX + TILE_SIZE) - (position.x - size.x/2);
                velocity.x = 0;
                collidedLeft = true;
            }
            // check right collision
            worldToTileCoordinates(position.x + size.x/2, position.y, &gridX, &gridY);
            if (map.mapData[gridY][gridX]+1 == 4 || map.mapData[gridY][gridX]+1 == 7 || map.mapData[gridY][gridX]+1 == 18 || map.mapData[gridY][gridX]+1 == 34) {
                position.x += (TILE_SIZE * gridX) - (position.x + size.x/2);
                velocity.x = 0;
                collidedLeft = true;
            }
        }
    }
    bool isColliding(Entity entity) const {
        if (fabs(position.x - entity.position.x) < (size.x/2 + entity.size.x/2) &&
            fabs(position.y - entity.position.y) < (size.y/2 + entity.size.y/2)) {
            return true;
        }
        return false;
    }

    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 size;

    SheetSprite sprite;

    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
};

class Level {
public:
    Level()
        : player(0.7, -1.9, 0.2, 0.2), key(4.5, -0.3, 0.2, 0.2){}
    void draw(ShaderProgram &p) const {
        player.draw(p);
        key.draw(p);
    }
    void update(float elapsed) {
        player.update(elapsed);
        if (player.isColliding(key)) {
            key.position.x = -1000;
            key.position.y = -1000;
            std::cout << "You got the key!" << std::endl;
        }
    }
    
    Entity player;
    Entity key;
};

Level level;

// Helper functions
void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    glViewport(0, 0, 960, 720);
    projectionMatrix = glm::ortho(-1.333, 1.333, -1.0, 1.0, -1.0, 1.0);
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glUseProgram(program.programID);
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    map.Load(RESOURCE_FOLDER"tileMap.txt");
    mapSpriteID = LoadTexture(RESOURCE_FOLDER"mapSprite.png");
    level.key.sprite = SheetSprite(mapSpriteID, 86, 16, 8);
    GLuint dinoSpriteID = LoadTexture(RESOURCE_FOLDER"dinoSprite.png");
    level.player.sprite = SheetSprite(dinoSpriteID, 0, 24, 1);
    

    for (int y=0; y < map.mapHeight; y++) {
        for (int x=0; x < map.mapWidth; x++) {
            if (map.mapData[y][x] != 0) {
                float u = (float)(((int)map.mapData[y][x]) % 16) / (float) 16;
                float v = (float)(((int)map.mapData[y][x]) / 16) / (float) 8;
                float spriteWidth = 1.0f/(float)16;
                float spriteHeight = 1.0f/(float)8;
                tileMapVertices.insert(tileMapVertices.end(), {
                    TILE_SIZE * x, -TILE_SIZE * y,
                    TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    TILE_SIZE * x, -TILE_SIZE * y,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
                });
                tileMapTexCoords.insert(tileMapTexCoords.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+(spriteHeight),
                    u, v,
                    u+spriteWidth, v+(spriteHeight),
                    u+spriteWidth, v
                });
            }
        }
    }
}

void ProcessEvents() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            gameDone = true;
        }
        // Player movement
        if (keys[SDL_SCANCODE_RIGHT]) {
            level.player.acceleration.x = 4.0;
        } else if (keys[SDL_SCANCODE_LEFT]) {
            level.player.acceleration.x = -4.0;
        } else {
            level.player.acceleration.x = 0;
        }
        if (keys[SDL_SCANCODE_UP] && level.player.collidedBottom) {
            level.player.velocity.y = 2.5;
        }
    }
}

void Update(float elapsed) {
    level.update(elapsed);
}

void Render() {
    glClearColor(0.07, 0.57, 0.65, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    program.SetViewMatrix(glm::translate(viewMatrix, -level.player.position + glm::vec3(0, -0.3, 0)));

    // Draw level
    level.draw(program);

    // Draw tilemap
    program.SetModelMatrix(modelMatrix);
    glEnableVertexAttribArray(program.positionAttribute);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glBindTexture(GL_TEXTURE_2D, mapSpriteID);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, tileMapVertices.data());
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, tileMapTexCoords.data());
    glDrawArrays(GL_TRIANGLES, 0, tileMapVertices.size()/2);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    // Display
    SDL_GL_SwapWindow(displayWindow);
    glFlush();
}

// Main
int main(int argc, char *argv[]) {
    Setup();
#ifdef _WINDOWS
    glewInit();
#endif
    while (!gameDone) {
        float ticks = SDL_GetTicks()/1000.0;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        elapsed += accumulator;
        ProcessEvents();
        if (elapsed < FIXED_TIMESTEP) {
            accumulator = elapsed;
            continue;
        }
        while (elapsed >= FIXED_TIMESTEP) {
            Update(FIXED_TIMESTEP);
            elapsed -= FIXED_TIMESTEP;
        }
        // rendering causes a lot of fps drops... why?
        // how to draw tilemap without fps drops?
        Render();
    }
    SDL_Quit();
    return 0;
}
