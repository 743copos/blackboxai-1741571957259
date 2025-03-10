#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include "raylib.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <random>
#include <algorithm>

// Time management
class DeltaTime {
private:
    static float deltaTime;
    static std::chrono::steady_clock::time_point lastTime;

public:
    static void Update() {
        auto currentTime = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
    }

    static float Get() { return deltaTime; }
};

float DeltaTime::deltaTime = 0.0f;
std::chrono::steady_clock::time_point DeltaTime::lastTime = std::chrono::steady_clock::now();

// Component system
struct Component {
    virtual ~Component() = default;
};

// Animation component
struct AnimationComponent : public Component {
    Texture2D spriteSheet;
    Rectangle frameRect;
    float frameTime = 0;
    float frameDuration = 0.1f;
    int currentFrame = 0;
    int frameCount = 1;
    bool loop = true;
    bool playing = true;

    void Update() {
        if (!playing) return;
        
        frameTime += DeltaTime::Get();
        if (frameTime >= frameDuration) {
            frameTime = 0;
            currentFrame++;
            if (currentFrame >= frameCount) {
                if (loop) currentFrame = 0;
                else {
                    currentFrame = frameCount - 1;
                    playing = false;
                }
            }
            frameRect.x = frameRect.width * currentFrame;
        }
    }
};

// Particle component
struct ParticleEmitter : public Component {
    struct Particle {
        Vector2 position;
        Vector2 velocity;
        float lifetime;
        float maxLifetime;
        Color color;
        float size;
        bool active = true;
    };

    std::vector<Particle> particles;
    Vector2 offset{0, 0};
    float emitRate = 10;
    float emitTimer = 0;
    float particleLifetime = 1.0f;
    Color particleColor = WHITE;
    float particleSpeed = 100.0f;
    bool emitting = true;

    void Update(const Vector2& emitterPos) {
        // Update existing particles
        for (auto& p : particles) {
            if (!p.active) continue;

            p.lifetime -= DeltaTime::Get();
            if (p.lifetime <= 0) {
                p.active = false;
                continue;
            }

            float lifePercent = p.lifetime / p.maxLifetime;
            p.color.a = static_cast<unsigned char>(255 * lifePercent);
            
            p.position.x += p.velocity.x * DeltaTime::Get();
            p.position.y += p.velocity.y * DeltaTime::Get();
            
            // Add gravity effect
            p.velocity.y += 200.0f * DeltaTime::Get();
        }

        // Emit new particles
        if (emitting) {
            emitTimer += DeltaTime::Get();
            if (emitTimer >= 1.0f / emitRate) {
                EmitParticle(emitterPos);
                emitTimer = 0;
            }
        }

        // Remove dead particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return !p.active; }),
            particles.end());
    }

    void EmitParticle(const Vector2& emitterPos) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> angleDist(-PI, PI);
        std::uniform_real_distribution<float> speedDist(0.5f, 1.0f);
        std::uniform_real_distribution<float> sizeDist(2.0f, 5.0f);

        Particle p;
        p.position = Vector2{emitterPos.x + offset.x, emitterPos.y + offset.y};
        
        float angle = angleDist(gen);
        float speed = particleSpeed * speedDist(gen);
        p.velocity = Vector2{cos(angle) * speed, sin(angle) * speed};
        
        p.lifetime = p.maxLifetime = particleLifetime;
        p.color = particleColor;
        p.size = sizeDist(gen);
        
        particles.push_back(p);
    }

    void Draw() {
        for (const auto& p : particles) {
            if (p.active) {
                DrawCircle(
                    static_cast<int>(p.position.x),
                    static_cast<int>(p.position.y),
                    p.size,
                    p.color
                );
            }
        }
    }
};

class Entity {
private:
    std::unordered_map<std::string, std::shared_ptr<Component>> components;

public:
    Vector2 position{0, 0};
    Vector2 size{32, 32};
    Color color{WHITE};
    bool active{true};
    float rotation{0.0f};
    Vector2 velocity{0, 0};
    Vector2 acceleration{0, 0};
    std::string tag;
    
    template<typename T>
    void AddComponent(const std::string& name, std::shared_ptr<T> component) {
        components[name] = component;
    }

    template<typename T>
    std::shared_ptr<T> GetComponent(const std::string& name) {
        auto it = components.find(name);
        if (it != components.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }
    
    virtual void Update() {
        // Apply acceleration
        velocity.x += acceleration.x * DeltaTime::Get();
        velocity.y += acceleration.y * DeltaTime::Get();
        
        // Apply velocity
        position.x += velocity.x * DeltaTime::Get();
        position.y += velocity.y * DeltaTime::Get();

        // Update components
        for (auto& [name, component] : components) {
            if (auto anim = std::dynamic_pointer_cast<AnimationComponent>(component)) {
                anim->Update();
            }
            if (auto emitter = std::dynamic_pointer_cast<ParticleEmitter>(component)) {
                emitter->Update(position);
            }
        }
    }

    virtual void Draw() {
        // Draw particle effects first
        if (auto emitter = GetComponent<ParticleEmitter>("particles")) {
            emitter->Draw();
        }

        // Draw sprite or animation
        if (auto anim = GetComponent<AnimationComponent>("animation")) {
            DrawTexturePro(
                anim->spriteSheet,
                anim->frameRect,
                Rectangle{position.x, position.y, size.x, size.y},
                Vector2{size.x/2, size.y/2},
                rotation,
                color
            );
        } else {
            DrawRectanglePro(
                Rectangle{position.x, position.y, size.x, size.y},
                Vector2{size.x/2, size.y/2},
                rotation,
                color
            );
        }
    }
    
    bool CheckCollision(const Entity& other) {
        return CheckCollisionRecs(GetBounds(), other.GetBounds());
    }

    Rectangle GetBounds() const {
        return Rectangle{
            position.x - size.x/2,
            position.y - size.y/2,
            size.x,
            size.y
        };
    }
};

class Scene {
private:
    std::vector<std::shared_ptr<Entity>> entities;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Entity>>> taggedEntities;

public:
    void AddEntity(std::shared_ptr<Entity> entity) {
        entities.push_back(entity);
        if (!entity->tag.empty()) {
            taggedEntities[entity->tag].push_back(entity);
        }
    }

    std::vector<std::shared_ptr<Entity>> GetEntitiesByTag(const std::string& tag) {
        return taggedEntities[tag];
    }

    void Update() {
        DeltaTime::Update();
        for(auto it = entities.begin(); it != entities.end();) {
            if((*it)->active) {
                (*it)->Update();
                ++it;
            } else {
                if (!(*it)->tag.empty()) {
                    auto& taggedList = taggedEntities[(*it)->tag];
                    auto taggedIt = std::find(taggedList.begin(), taggedList.end(), *it);
                    if (taggedIt != taggedList.end()) {
                        taggedList.erase(taggedIt);
                    }
                }
                it = entities.erase(it);
            }
        }
    }

    void Draw() {
        for(auto& entity : entities) {
            if(entity->active) {
                entity->Draw();
            }
        }
    }

    std::vector<std::shared_ptr<Entity>>& GetEntities() {
        return entities;
    }
};

class GameEngine {
private:
    int screenWidth;
    int screenHeight;
    std::string title;
    Scene currentScene;
    bool debugMode = false;

public:
    GameEngine(int width, int height, const std::string& windowTitle) 
        : screenWidth(width), screenHeight(height), title(windowTitle) {
        InitWindow(screenWidth, screenHeight, title.c_str());
        SetTargetFPS(60);
    }

    ~GameEngine() {
        CloseWindow();
    }

    bool ShouldClose() {
        return WindowShouldClose();
    }

    void Clear() {
        BeginDrawing();
        ClearBackground(BLACK);
    }

    void Display() {
        if (debugMode) {
            DrawFPS(10, 10);
            for (auto& entity : currentScene.GetEntities()) {
                if (entity->active) {
                    DrawRectangleLinesEx(entity->GetBounds(), 1, GREEN);
                }
            }
        }
        EndDrawing();
    }

    void ToggleDebugMode() {
        debugMode = !debugMode;
    }

    void Update() {
        currentScene.Update();
    }

    void Draw() {
        currentScene.Draw();
    }

    Scene& GetCurrentScene() {
        return currentScene;
    }

    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }
};

#endif
