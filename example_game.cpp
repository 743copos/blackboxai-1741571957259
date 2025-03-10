#include "GameEngine.hpp"

// Example sprite component
struct SpriteComponent : public Component {
    Texture2D texture;
    SpriteComponent(const std::string& path) {
        texture = ResourceManager::LoadTexture(path);
    }
};

// Example player class with improved features
class Player : public Entity {
public:
    Player() {
        size = Vector2{50, 50};
        color = RED;
        position = Vector2{400, 300};
        tag = "player";  // Tag for easy lookup
    }

    void Update() override {
        // Frame-independent movement
        const float speed = 300.0f;  // pixels per second
        
        if (IsKeyDown(KEY_RIGHT)) velocity.x = speed;
        else if (IsKeyDown(KEY_LEFT)) velocity.x = -speed;
        else velocity.x = 0;

        if (IsKeyDown(KEY_DOWN)) velocity.y = speed;
        else if (IsKeyDown(KEY_UP)) velocity.y = -speed;
        else velocity.y = 0;

        // Call parent update for physics
        Entity::Update();
    }
};

// Example enemy class
class Enemy : public Entity {
public:
    Enemy(float x, float y) {
        size = Vector2{30, 30};
        color = BLUE;
        position = Vector2{x, y};
        tag = "enemy";
    }

    void Update() override {
        rotation += 45.0f * DeltaTime::Get();  // Rotate 45 degrees per second
        Entity::Update();
    }
};

int main() {
    GameEngine engine(800, 600, "Enhanced Example Game");
    Scene& scene = engine.GetCurrentScene();

    // Create player
    auto player = std::make_shared<Player>();
    scene.AddEntity(player);

    // Create some enemies
    scene.AddEntity(std::make_shared<Enemy>(200, 200));
    scene.AddEntity(std::make_shared<Enemy>(600, 400));

    // Subscribe to collision events
    scene.GetEventSystem().Subscribe("collision", [](void* data) {
        // Handle collision event
        printf("Collision detected!\n");
    });

    // Game loop
    while (!engine.ShouldClose()) {
        engine.Clear();

        // Toggle debug mode with F1
        if (IsKeyPressed(KEY_F1)) {
            engine.ToggleDebugMode();
        }

        // Check collisions between player and enemies
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
