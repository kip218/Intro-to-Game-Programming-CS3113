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
float lastFrameTicks = 0.0;
float accumulator = 0.0;
glm::vec3 gravity(0.0, -2.0, 0.0);
const Uint8 *keys = SDL_GetKeyboardState(NULL);


/*
 Classes
*/
class Entity {
public:
    Entity(float x, float y, float width, float height, float r, float g, float b)
    : position(x, y, 0), size(width, height, 1), color(r, g, b) {}
    void draw(ShaderProgram &p) const {
        glm::mat4 entityModelMatrix = glm::translate(modelMatrix, position);
        entityModelMatrix = glm::scale(entityModelMatrix, size);
        p.SetModelMatrix(entityModelMatrix);
        float vertices[] = {-0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5};
        glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(p.positionAttribute);
        p.SetColor(color.r, color.g, color.b, 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(p.positionAttribute);
    }
    
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 color;
};

class MovingEntity : public Entity {
public:
    MovingEntity(float x, float y, float width, float height, float r, float g, float b)
        : Entity(x, y, width, height, r, g, b), velocity(0, 0, 0), acceleration(0, 0, 0) {}
    void update(float elapsed, const std::vector<Entity> &platforms, const std::vector<Entity> &obstacles) {
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
        // check y collision
        position.y += velocity.y * elapsed;
        adjustCollisionsY(platforms);
        adjustCollisionsY(obstacles);
    }
    void process(const Uint8 *keys) {
        if (keys[SDL_SCANCODE_SPACE] && collidedBottom) {
            velocity.y = 1.25;
        }

        if (keys[SDL_SCANCODE_RIGHT]) {
            velocity.x = 0.5;
        } else if (keys[SDL_SCANCODE_LEFT]) {
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
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
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
    Level() : player(0, 1.0, 0.1, 0.1, 0.75, 0.33, 0.33), camera(0, 0, 3.2, 2.0, 0.25, 0) {
        // background panels
        background.push_back(Entity(-1.8, 0, 0.4, 2.0, 0.26, 0.26, 0.26));
        background.push_back(Entity(-1.4, 0, 0.4, 2.0, 0.4, 0.4, 0.4));
        background.push_back(Entity(-1.0, 0, 0.4, 2.0, 0.26, 0.26, 0.26));
        background.push_back(Entity(-0.6, 0, 0.4, 2.0, 0.4, 0.4, 0.4));
        background.push_back(Entity(-0.2, 0, 0.4, 2.0, 0.26, 0.26, 0.26));
        background.push_back(Entity(0.2, 0, 0.4, 2.0, 0.4, 0.4, 0.4));
        background.push_back(Entity(0.6, 0, 0.4, 2.0, 0.26, 0.26, 0.26));
        background.push_back(Entity(1.0, 0, 0.4, 2.0, 0.4, 0.4, 0.4));
        background.push_back(Entity(1.4, 0, 0.4, 2.0, 0.26, 0.26, 0.26));
        background.push_back(Entity(1.8, 0, 0.4, 2.0, 0.4, 0.4, 0.4));
        left_panel_i = 0;
        right_panel_i = background.size() - 1;
        // platforms
        platforms.push_back(Entity(0, -10.5, 6.4, 20, 0.30, 0.45, 0.55));
        platforms.push_back(Entity(6.4, -10.5, 6.4, 20, 0.30, 0.45, 0.55));
        left_platform_i = 0;
        right_platform_i = platforms.size() - 1;
        // obstacles
        obstacles.push_back(Entity(1.0, -0.45, 0.1, 0.1, 0.75, 0.75, 0.33));
        obstacles.push_back(Entity(0.5, -0.2, 0.1, 0.1, 0.75, 0.75, 0.33));
        obstacles.push_back(Entity(6.4, -0.2, 0.1, 0.1, 0.75, 0.75, 0.33));
        obstacles.push_back(Entity(12.8, -0.2, 0.1, 0.1, 0.75, 0.75, 0.33));
    }
    void draw(ShaderProgram &p) const {
        camera.setViewMatrix(p);
        for (const Entity &panel : background) {
            panel.draw(p);
        }
        player.draw(p);
        for (const Entity &platform : platforms) {
            platform.draw(p);
        }
        for (const Entity &obstacle : obstacles) {
            obstacle.draw(p);
        }
    }
    void update(float elapsed) {
        camera.move(elapsed);
        player.update(elapsed, platforms, obstacles);
        // move background panels from left to right to reuse them in camera view
        if (background[left_panel_i].position.x + background[left_panel_i].size.x/2
            < camera.position.x - camera.size.x/2) {
            background[left_panel_i].position.x = background[right_panel_i].position.x + background[left_panel_i].size.x;
            left_panel_i++;
            right_panel_i++;
            if (left_panel_i >= background.size()) { left_panel_i = 0; }
            if (right_panel_i >= background.size()) { right_panel_i = 0; }
        }
        // move platforms from left to right to reuse them in camera view
        if (platforms[left_platform_i].position.x + platforms[left_platform_i].size.x/2
            < camera.position.x - camera.size.x/2) {
            platforms[left_platform_i].position.x = platforms[right_platform_i].position.x + platforms[left_platform_i].size.x;
            left_platform_i++;
            right_platform_i++;
            if (left_platform_i >= platforms.size()) { left_platform_i = 0; }
            if (right_platform_i >= platforms.size()) { right_platform_i = 0; }
        }
    }
    void process(const Uint8 *keys) {
        player.process(keys);
    }
    MovingEntity player;
    std::vector<Entity> background;
    std::vector<Entity> platforms;
    std::vector<Entity> obstacles;
    Camera camera;
    size_t left_panel_i;
    size_t right_panel_i;
    size_t left_platform_i;
    size_t right_platform_i;
};

Level level;


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
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || keys[SDL_SCANCODE_ESCAPE]) {
            gameDone = true;
        }
    }
    level.process(keys);
}

void update(float elapsed) {
    level.update(elapsed);
}

void render() {
    glClearColor(0.6, 0.6, 0.6, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    level.draw(program);
    
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
    SDL_Quit();
    return 0;
}
