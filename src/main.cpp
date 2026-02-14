#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <entt/entt.hpp>
#include <random>

// Components
struct Position {
    sf::Vector2f value;
};

struct Velocity {
    sf::Vector2f value;
};

// Holds an sf::Sprite. We keep the texture externally (global or in main).
struct SpriteComp {
    sf::Sprite sprite;
};

// Simple helper to generate random floats in [min, max]
float randomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

int main() {
    std::string title = "SFML3 + EnTT Test";

    // Create the window
    sf::RenderWindow window(sf::VideoMode({800u, 600u}), title, sf::Style::Default);
    window.setVerticalSyncEnabled(true);

    // Load a texture (from the examples resources)
    sf::Texture texture;
    if (!texture.loadFromFile("resources/warn.png")) {
        return 1;
    }

    // Create the EnTT registry
    entt::registry registry;

    // Create a player entity
    const auto player = registry.create();
    registry.emplace<Position>(player, sf::Vector2f(400.f, 300.f));
    registry.emplace<Velocity>(player, sf::Vector2f(0.f, 0.f));

    sf::Sprite playerSprite(texture);
    playerSprite.setColor(sf::Color::Green);        // tint green to distinguish
    playerSprite.setOrigin(sf::Vector2f(texture.getSize()) / 2.f);
    registry.emplace<SpriteComp>(player, std::move(playerSprite));

    // Create several ball entities with random positions/velocities
    constexpr std::size_t ballCount = 10;
    for (std::size_t i = 0; i < ballCount; ++i) {
        const auto ball = registry.create();
        sf::Vector2f pos(randomFloat(50.f, 750.f), randomFloat(50.f, 550.f));
        sf::Vector2f vel(randomFloat(-200.f, 200.f), randomFloat(-200.f, 200.f));
        registry.emplace<Position>(ball, pos);
        registry.emplace<Velocity>(ball, vel);

        sf::Sprite ballSprite(texture);
        ballSprite.setColor(sf::Color::Red);        // tint red
        ballSprite.setOrigin(sf::Vector2f(texture.getSize()) / 2.f);
        // Scale down a bit so balls are smaller
        ballSprite.setScale({0.5f, 0.5f});
        registry.emplace<SpriteComp>(ball, std::move(ballSprite));
    }

    sf::Clock clock;

    // Main loop
    while (window.isOpen()) {
        // Delta time
        const float dt = clock.restart().asSeconds();

        // Event handling (using SFML 3's std::optional style)
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
        }

        // Player input (arrow keys)
        sf::Vector2f playerVel(0.f, 0.f);
        constexpr float playerSpeed = 300.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            playerVel.x = -playerSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            playerVel.x = playerSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            playerVel.y = -playerSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            playerVel.y = playerSpeed;

        // Update player velocity component
        auto& playerVelComp = registry.get<Velocity>(player);
        playerVelComp.value = playerVel;

        // Movement system: update position based on velocity
        auto moveView = registry.view<Position, Velocity>();
        for (auto [entity, pos, vel] : moveView.each()) {
            pos.value += vel.value * dt;

            // Simple screen wrapping for balls (except player)
            if (entity != player) {
                if (pos.value.x < 0.f) pos.value.x = 800.f;
                if (pos.value.x > 800.f) pos.value.x = 0.f;
                if (pos.value.y < 0.f) pos.value.y = 600.f;
                if (pos.value.y > 600.f) pos.value.y = 0.f;
            } else {
                // Keep player inside window
                pos.value.x = std::clamp(pos.value.x, 0.f, 800.f);
                pos.value.y = std::clamp(pos.value.y, 0.f, 600.f);
            }
        }

        // Rendering system: update sprite positions and draw
        window.clear(sf::Color::Black);
        auto drawView = registry.view<Position, SpriteComp>();
        for (auto [entity, pos, spr] : drawView.each()) {
            spr.sprite.setPosition(pos.value);
            window.draw(spr.sprite);
        }
        window.display();
    }

    return 0;
}