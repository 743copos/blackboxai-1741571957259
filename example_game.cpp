#include "GameEngine.hpp"
#include <iostream>

class Player : public Entity {
public:
    Player() {
        size = Vector2{50, 50};
        color = WHITE;
        position = Vector2{400, 300};
        tag = "player";

        // Add thrust particles
        auto particles = std::make_shared<ParticleEmitter>();
        particles->particleColor = ORANGE;
        particles->particleLifetime = 0.5f;
        particles->emitRate = 20;
        particles->particleSpeed = 50.0f;
        particles->offset = Vector2{-25, 0};
        AddComponent("particles", particles);
    }

    void Update() override {
        velocity = {0, 0};
        const float speed = 300.0f;
        
        if (IsKeyDown(KEY_RIGHT)) {
            velocity.x = speed;
            rotation = 0;
        }
        else if (IsKeyDown(KEY_LEFT)) {
            velocity.x = -speed;
            rotation = 180;
        }

        if (IsKeyDown(KEY_DOWN)) {
            velocity.y = speed;
            rotation = 90;
        }
        else if (IsKeyDown(KEY_UP)) {
            velocity.y = -speed;
            rotation = 270;
        }

        // Enable particles when moving
        if (auto particles = GetComponent<ParticleEmitter>("particles")) {
            particles->emitting = (velocity.x != 0 || velocity.y != 0);
        }

        Entity::Update();

        // Screen bounds
        if (position.x < size.x/2) position.x = size.x/2;
        if (position.x > 800 - size.x/2) position.x = 800 - size.x/2;
        if (position.y < size.y/2) position.y = size.y/2;
        if (position.y > 600 - size.y/2) position.y = 600 - size.y/2;
    }
};

class Enemy : public Entity {
private:
    float time = 0;
    float explosionTimer = 0;
    bool exploding = false;

public:
    Enemy(float x, float y) {
        size = Vector2{40, 40};
        color = RED;
        position = Vector2{x, y};
        tag = "enemy";

        // Add explosion particles
        auto particles = std::make_shared<ParticleEmitter>();
        particles->particleColor = YELLOW;
        particles->particleLifetime = 1.0f;
        particles->emitRate = 0;
        particles->particleSpeed = 200.0f;
        AddComponent("particles", particles);
    }

    void Explode() {
        if (!exploding) {
            exploding = true;
            if (auto particles = GetComponent<ParticleEmitter>("particles")) {
                particles->emitRate = 100;
                particles->emitting = true;
            }
        }
    }

    void Update() override {
        if (exploding) {
            explosionTimer += DeltaTime::Get();
            if (explosionTimer >= 1.0f) {
                active = false;
                return;
            }
            color.a = static_cast<unsigned char>(255 * (1.0f - explosionTimer));
        } else {
            // Circular movement
            time += DeltaTime::Get();
            float radius = 100.0f;
            position.x = 400 + radius * cos(time);
            position.y = 300 + radius * sin(time);
            rotation += 90.0f * DeltaTime::Get();
        }
        
        Entity::Update();
    }
};

int main() {
    GameEngine engine(800, 600, "Enhanced Game Demo");
    Scene& scene = engine.GetCurrentScene();

    // Create player
    auto player = std::make_shared<Player>();
    scene.AddEntity(player);

    // Create enemies
    scene.AddEntity(std::make_shared<Enemy>(200, 200));
    scene.AddEntity(std::make_shared<Enemy>(600, 400));

    // Game loop
    while (!engine.ShouldClose()) {
        engine.Clear();

        // Toggle debug mode
        if (IsKeyPressed(KEY_F1)) {
            engine.ToggleDebugMode();
        }

        // Check collisions and trigger explosions
        auto enemies = scene.GetEntitiesByTag("enemy");
        for (auto& enemy : enemies) {
            if (player->CheckCollision(*enemy)) {
                std::static_pointer_cast<Enemy>(enemy)->Explode();
            }
        }

        engine.Update();
        engine.Draw();
        engine.Display();
    }

    return 0;
}
