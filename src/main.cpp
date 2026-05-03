#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <format>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include "core/events.hpp"
#include "core/system.hpp"
#include "editor/components.hpp"
#include "graphics/components.hpp"
#include "graphics/texture.hpp"
#include "input/components.hpp"
#include "input/events.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "serialization/serialization.hpp"
#include "ui/builder.hpp"
#include "ui/components.hpp"
#include "utility/math.hpp"
#include "world/components.hpp"
#include "yaml-cpp/node/parse.h"

void genWorld(entt::registry& registry);
void genUI(entt::registry& registry);

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

    init_systems(registry);

    // try load saved registry
    if (std::filesystem::exists("save.yml")) {
        YAML::Node saveNode = YAML::LoadFile("save.yml");
        deserialize_registry(saveNode, registry);
    } else {
    // else generate
        genWorld(registry);
    }
    genUI(registry);

    sf::Clock clock;

    // Initial resize event
    ScreenResizeEvent iScreenEv{window.getSize().x, window.getSize().y, &registry};
    dispatcher.trigger(iScreenEv);

    // Main loop
    while (window.isOpen()) {
        // Delta time
        const float dt = clock.restart().asSeconds();

        // Event handling
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            if (auto* mouseEv = event->getIf<sf::Event::MouseButtonPressed>()) {
                GlobalClickEvent clickEv(mouseEv->position, mouseEv->button, true, &registry);
                dispatcher.trigger(clickEv);
                continue;
            }
            if (auto* mouseEv = event->getIf<sf::Event::MouseButtonReleased>()) {
                GlobalClickEvent clickEv(mouseEv->position, mouseEv->button, false, &registry);
                dispatcher.trigger(clickEv);
                continue;
            }
            if (auto* wheelEv = event->getIf<sf::Event::MouseWheelScrolled>()) {
                GlobalScrollEvent scrollEv(wheelEv->position, wheelEv->delta, &registry);
                dispatcher.trigger(scrollEv);
                continue;
            }
            if (auto* moveEv = event->getIf<sf::Event::MouseMoved>()) {
                GlobalMouseMoveEvent mouseEv(moveEv->position, &registry);
                dispatcher.trigger(mouseEv);
                continue;
            }
            if (auto* resizeEv = event->getIf<sf::Event::Resized>()) {
                ScreenResizeEvent screenEv{resizeEv->size.x, resizeEv->size.y, &registry};
                dispatcher.trigger(screenEv);
                continue;
            }
            if (auto* keyEv = event->getIf<sf::Event::KeyPressed>()) {
                KeyPressEvent pressEv{keyEv->code, &registry};
                dispatcher.trigger(pressEv);
                continue;
            }
        }

        // Dispatch update event
        UpdateEvent updateEv(dt, &registry);
        dispatcher.trigger(updateEv);
    }

    std::cout << std::format("Writing save to save.yml...");
    std::ofstream save_s("save.yml");
    save_s << serialize_registry(registry);

    return 0;
}

void spawnBall(entt::registry& registry, entt::entity world) {
    const auto ball = registry.create();
    float size_m = math::rand(0.5f, 1.5f);
    sf::Vector2f pos(math::randf(50.f, 750.f), math::randf(50.f, 550.f));
    sf::Vector2f vel(math::randf(-200.f, 200.f), math::randf(-200.f, 200.f));
    registry.emplace<PositionComp>(ball, pos, world);
    registry.emplace<PhysicsComp>(ball, vel, 10.f);
    sf::Vector2f size = sf::Vector2f{32.f, 32.f} * size_m;
    registry.emplace<BoundsComp>(ball, sf::FloatRect{-0.5f * size, size});
    registry.emplace<ClickListenerComp>(ball);
    registry.emplace<ButtonComp>(ball, [](ClickEvent&) { std::cout << "test" << std::endl; });
    registry.emplace<ColliderComp>(ball, CollisionLayer::Player, CollisionLayer::Wall | CollisionLayer::Player, 5.f * size_m * size_m);

    sf::Sprite ballSprite(tex_map["mob"]);
    ballSprite.setColor(sf::Color::Red);        // tint red
    ballSprite.setOrigin(sf::Vector2f(tex_map["mob"].getSize()) / 2.f);
    // Scale down a bit so balls are smaller
    ballSprite.setScale({size_m, size_m});
    registry.emplace<SpriteComp>(ball, std::move(ballSprite));
    registry.emplace<TextComp>(ball, sf::Text(font_map["hack"], "test", 15));
    registry.emplace<RenderableComp>(ball, z_entity);
}

void genWorld(entt::registry& registry) {
    entt::entity world = registry.create();
    registry.emplace<PositionComp>(world, sf::Vector2f(0.f, 0.f), world);
    TileMapComp& mapComp = registry.emplace<TileMapComp>(world, 64, 64, 32.f, &tex_map["tileset"]);
    registry.emplace<RenderableComp>(world, z_world);
    registry.emplace<BoundsComp>(world);
    // Add some random walls
    mapComp.grid.resize(mapComp.height * mapComp.width);
    for (int i = 0; i < 200; ++i) {
        int x = math::rand(0, mapComp.width - 1);
        int y = math::rand(0, mapComp.height - 1);

        entt::entity wall = registry.create();
        registry.emplace<PositionComp>(wall, sf::Vector2f(x * 32.f + 16.f, y * 32.f + 16.f), world);
        registry.emplace<BoundsComp>(wall, sf::FloatRect{{-16.f, -16.f}, {32.f, 32.f}});
        registry.emplace<ColliderComp>(wall, CollisionLayer::Wall, CollisionLayer::None);
        registry.emplace<RenderableComp>(wall, z_world + 1);
        sf::Sprite wallSprite(tex_map["wall"]);
        wallSprite.setOrigin(sf::Vector2f(tex_map["wall"].getSize()) / 2.f);
        registry.emplace<SpriteComp>(wall, std::move(wallSprite));
    }
    // Generate the efficient vertex array
    MapUtil::rebuildMesh(world, mapComp, registry);

    // Create a player entity
    entt::entity player = registry.create();
    registry.emplace<PositionComp>(player, sf::Vector2f(0.f, 0.f), world);
    registry.emplace<PhysicsComp>(player, sf::Vector2f(0.f, 0.f), 1600.f);
    registry.emplace<BoundsComp>(player, sf::FloatRect{{-16.f, -16.f}, {32.f, 32.f}});
    registry.emplace<InputMovementComp>(player, 600.f, 3000.f);
    registry.emplace<ColliderComp>(player, CollisionLayer::Player, CollisionLayer::Wall | CollisionLayer::Player);

    sf::Sprite playerSprite(tex_map["mob"]);
    playerSprite.setColor(sf::Color::Green); // tint green to distinguish
    playerSprite.setOrigin(sf::Vector2f(tex_map["mob"].getSize()) / 2.f);
    registry.emplace<SpriteComp>(player, std::move(playerSprite));
    registry.emplace<RenderableComp>(player, z_entity);

    const auto camera = registry.create();
    registry.emplace<PositionComp>(camera, sf::Vector2f(0.f, 0.f), player);
    registry.emplace<CameraComp>(camera, 1.f, 0);
    registry.emplace<MainCameraComp>(camera);

    // Create several ball entities with random positions/velocities
    constexpr std::size_t ballCount = 10;
    for (std::size_t i = 0; i < ballCount; ++i) {
        spawnBall(registry, world);
    }
}

void genUI(entt::registry& registry) {
    UIBuilder uiWorld = UIBuilder::makeWorld(registry, "UI World");

    UIBuilder sidePanel =
        uiWorld.child("Test Panel")
            .posAbsolute({{20.f, 100.f}, {200.f, 400.f}})
            .window(sf::Color(35, 35, 45, 155),
                sf::Color(130, 130, 200),
                sf::Color(70, 70, 100), 2.f, 30.f)
            .allocatorTile({48.f, 48.f}, 2.f, 2.f, {0.f, 0.f, 180.f, 370.f, {false, false, false, false}})
            .buttonToggled(sf::Keyboard::Key::F3)
            .scrollable({180.f, 0.f, 20.f, 370.f, {false, false, false, false}}, sf::Color(140, 140, 150, 128), sf::Color(100, 100, 110, 196), sf::Color(0, 0, 0))
            .draggable(0.f, 370.f, 200.f, 30.f, {false, false, false, false})
            .stencil({2.f, 2.f, 176.f, 366.f, {false, false, false, false}});

    // Side panel items
    for (int i = 0; i < 50; ++i) {
        std::string name = "Panel " + std::to_string(i);
        sidePanel.child(name)
            .posFill()
            .rect(sf::Color(0, 0, 0, 0), sf::Color(120, 120, 140), 1.f)
            .text(name, "hack", 14)
            .stencil();
    }

    sidePanel.child("Spawn Button")
        .posFill()
        .rect(sf::Color(120, 0, 0, 128), sf::Color(120, 120, 140), 1.f)
        .button([](ClickEvent& click) {
            auto pView = click.registry->view<InputMovementComp>();
            entt::entity world;
            for (auto [ent, mover] : pView.each()) {
                world = Physics::getWorld(ent, *click.registry);
                break;
            }
            spawnBall(*click.registry, world);
        });
}
