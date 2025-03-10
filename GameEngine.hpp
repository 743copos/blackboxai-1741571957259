#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include "raylib.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <algorithm>

// Time management for frame-independent movement
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

// Resource manager for textures
class ResourceManager {
private:
    static std::unordered_map<std::string, Texture2D> textures;

public:
    static Texture2D LoadTexture(const std::string& path) {
        if (textures.find(path) != textures.end()) {
            return textures[path];
        }
        Texture2D texture = ::LoadTexture(path.c_str());
        textures[path] = texture;
        return texture;
    }

    static void UnloadAll() {
        for (auto& [path, texture] : textures) {
            UnloadTexture(texture);
        }
        textures.clear();
    }
};

std::unordered_map<std::string, Texture2D> ResourceManager::textures;

// Component system for entities
struct Component {
    virtual ~Component() = default;
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
        // Frame-independent movement
        position.x += velocity.x * DeltaTime::Get();
        position.y += velocity.y * DeltaTime::Get();
    }

    virtual void Draw() {
        DrawRectanglePro(
            Rectangle{position.x, position.y, size.x, size.y},
            Vector2{size.x/2, size.y/2},
            rotation,
            color
        );
    }
    
    bool CheckCollision(const Entity& other) {
        return CheckCollisionRecs(
            Rectangle{position.x - size.x/2, position.y - size.y/2, size.x, size.y},
            Rectangle{other.position.x - other.size.x/2, other.position.y - other.size.y/2, other.size.x, other.size.y}
        );
    }

    Rectangle GetBounds() const {
        return Rectangle{position.x - size.x/2, position.y - size.y/2, size.x, size.y};
    }
};

// Event system
class EventSystem {
private:
    std::unordered_map<std::string, std::vector<std::function<void(void*)>>> eventHandlers;

public:
    void Subscribe(const std::string& eventName, std::function<void(void*)> handler) {
        eventHandlers[eventName].push_back(handler);
    }

    void Emit(const std::string& eventName, void* data = nullptr) {
        if (eventHandlers.find(eventName) != eventHandlers.end()) {
            for (auto& handler : eventHandlers[eventName]) {
                handler(data);
            }
        }
    }
};

class Scene {
private:
    std::vector<std::shared_ptr<Entity>> entities;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Entity>>> taggedEntities;
    EventSystem eventSystem;

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

    EventSystem& GetEventSystem() {
        return eventSystem;
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
        ResourceManager::UnloadAll();
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
