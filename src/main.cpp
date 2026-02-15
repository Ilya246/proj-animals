#include <SFML/Graphics/Rect.hpp>
#include <cstdlib>
#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <memory>
#include <queue>

#include "core/events.hpp"
#include "core/system.hpp"
#include "graphics/components.hpp"
#include "input/components.hpp"
#include "physics/components.hpp"
#include "utility/math.hpp"
#include "world/components.hpp"

int main() {
    // Create the EnTT registry
    entt::registry registry;
    entt::dispatcher dispatcher; // Primary event dispatcher

    std::string title = "SFML3 + EnTT Test";

    // Create the window
    sf::RenderWindow window(sf::VideoMode({800u, 600u}), title, sf::Style::Default);
    window.setVerticalSyncEnabled(true);

    registry.ctx().emplace<entt::dispatcher&>(dispatcher);
    registry.ctx().emplace<sf::RenderWindow&>(window);

    // Initialise systems
    std::priority_queue<std::pair<int, SystemBase*>> init_queue;
    for (auto& sys_ptr : systems) {
        init_queue.push({sys_ptr->initPriority(), sys_ptr.get()});
    }
    while (init_queue.size() != 0) {
        auto& sys = init_queue.top().second;
        sys->init(registry);
        init_queue.pop();
    }

    // Load a texture (from the examples resources)
    sf::Texture texture;
    if (!texture.loadFromFile("resources/copper-wall.png")) {
        return 1;
    }
    sf::Texture tileset;
    if (!tileset.loadFromFile("resources/tileset.png")) {
        return 1;
    }

    const auto world = registry.create();
    registry.emplace<PositionComp>(world, sf::Vector2f(0.f, 0.f), world);
    TileMapComp& mapComp = registry.emplace<TileMapComp>(world, 64, 64, 32.f, &tileset);
    // Add some random walls
    mapComp.grid.resize(mapComp.height * mapComp.width);
    for (int i = 0; i < 200; ++i) {
        int x = math::rand(0, mapComp.width - 1);
        int y = math::rand(0, mapComp.height - 1);
        mapComp.grid[x + y * mapComp.width] = TileType::Wall;
    }
    // Generate the efficient vertex array
    MapUtil::rebuildMesh(mapComp);

    // Create a player entity
    const auto player = registry.create();
    registry.emplace<PositionComp>(player, sf::Vector2f(64.f, 64.f), world);
    registry.emplace<PhysicsComp>(player, sf::Vector2f(0.f, 0.f), 1600.f, sf::FloatRect{{-16.f, -16.f}, {32.f, 32.f}});
    registry.emplace<InputMovementComp>(player, 600.f, 3000.f);

    sf::Sprite playerSprite(texture);
    playerSprite.setColor(sf::Color::Green); // tint green to distinguish
    playerSprite.setOrigin(sf::Vector2f(texture.getSize()) / 2.f);
    registry.emplace<SpriteComp>(player, std::move(playerSprite));

    const auto camera = registry.create();
    registry.emplace<PositionComp>(camera, sf::Vector2f(0.f, 0.f), player);
    registry.emplace<CameraComp>(camera, 1.f, sf::View());

    // Create several ball entities with random positions/velocities
    constexpr std::size_t ballCount = 10;
    for (std::size_t i = 0; i < ballCount; ++i) {
        const auto ball = registry.create();
        sf::Vector2f pos(math::randf(50.f, 750.f), math::randf(50.f, 550.f));
        sf::Vector2f vel(math::randf(-200.f, 200.f), math::randf(-200.f, 200.f));
        registry.emplace<PositionComp>(ball, pos, world);
        registry.emplace<PhysicsComp>(ball, vel, 10.f, sf::FloatRect{{-8.f, -8.f}, {16.f, 16.f}});

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

        // Dispatch update event
        UpdateEvent updateEv(dt, &registry);
        dispatcher.trigger(updateEv);
    }

    return 0;
}