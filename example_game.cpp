#include "GameEngine.hpp"
#include <iostream>

class Player : public Entity {
public:
    Player() {
        size = Vector2{50, 50};
        color = RED;
        position = Vector2{400, 300};
        tag = "player";
    }

    void Update() override {
        // Frame-independent movement
        const float speed = 300.0f;  // pixels per second
        
        // Reset velocity each frame
        velocity = {0, 0};
        
        if (IsKeyDown(KEY_RIGHT)) velocity.x = speed;
        else if (IsKeyDown(KEY_LEFT)) velocity.x = -speed;

        if (IsKeyDown(KEY_DOWN)) velocity.y = speed;
        else if (IsKeyDown(KEY_UP)) velocity.y = -speed;

        Entity::Update();

        // Keep player within screen bounds
        if (position.x < size.x/2) position.x = size.x/2;
        if (position.x > 800 - size.x/2) position.x = 800 - size.x/2;
        if (position.y < size.y/2) position.y = size.y/2;
        if (position.y > 600 - size.y/2) position.y = 600 - size.y/2;
    }
};

class Enemy : public Entity {
private:
    float time;

public:
    Enemy(float x, float y) {
        size = Vector2{30, 30};
        color = BLUE;
        position = Vector2{x, y};
        tag = "enemy";
        time = 0;
    }

    void Update() override {
        // Rotate 90 degrees per second
        rotation += 90.0f * DeltaTime::Get();
        
        // Circular movement
        time += DeltaTime::Get();
        float radius = 100.0f;
        position.x = 400 + radius * cos(time);
        position.y = 300 + radius * sin(time);
        
        Entity::Update();
    }
};

int main() {
    GameEngine engine(800, 600, "Game Engine Demo");
    Scene& scene = engine.GetCurrentScene();

    // Create player
    auto player = std::make_shared<Player>();
    scene.AddEntity(player);

    // Create enemies
    scene.AddEntity(std::make_shared<Enemy>(200, 200));
    scene.AddEntity(std::make_shared<Enemy>(600, 400));

    // Setup collision event
    scene.GetEventSystem().Subscribe("collision", [](void* data) {
        std::cout << "Collision detected!" << std::endl;
    });

    // Game loop
    while (!engine.ShouldClose()) {
        engine.Clear();

        // Toggle debug mode with F1
        if (IsKeyPressed(KEY_F1)) {
            engine.ToggleDebugMode();
        }

        // Check collisions
        auto enemies = scene.GetEntitiesByTag("enemy");
        for (auto& enemy : enemies) {
            if (player->CheckCollision(*enemy)) {
                scene.GetEventSystem().Emit("collision");
            }
        }

        engine.Update();
        engine.Draw();
        engine.Display();
    }

    return 0;
}
