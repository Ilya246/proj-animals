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

        // Event handling (using SFML 3's std::optional style)
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
            if (auto* moveEv = event->getIf<sf::Event::MouseMoved>()) {
                GlobalMouseMoveEvent mouseEv(moveEv->position, &registry);
                dispatcher.trigger(mouseEv);
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

    // Create several ball entities with random positions/velocities
    constexpr std::size_t ballCount = 10;
    for (std::size_t i = 0; i < ballCount; ++i) {
        spawnBall(registry, world);
    }
}

void genUI(entt::registry& registry) {
    // --- UI world root entity ---
    UIBuilder uiWorld = UIBuilder::makeWorld(registry, "UI World");

    // --- Top panel: a bordered panel at the top of the screen ---
    UIBuilder topPanel =
        uiWorld.child("Top Panel")
            .posAnchor(0.f, 0.8f, 1.f, 0.2f)
            .rect(sf::Color(30, 30, 30, 200), sf::Color(100, 100, 120), 2.f)
            .allocatorLayout(UILayoutMode::Horizontal, 0.f, 4.f);

    topPanel.child("Title Text")
        .posFill()
        .text("Animals UI Demo", "hack", 22);
    topPanel.child("Spacer")
        .posFill()
        .rect(sf::Color(100, 100, 120, 128), sf::Color(40, 40, 50), 4.f);
    topPanel.child("Status Text")
        .posFill()
        .text("Status: OK", "hack", 16, sf::Color::Green);

    // --- Side panel: vertical list on the left ---
    UIBuilder sidePanel =
        uiWorld.child("Side Panel")
            .posAnchor(0.f, 0.2f, 0.2f, 0.6f)
            .rect(sf::Color(40, 40, 50, 220), sf::Color(100, 100, 120), 1.f)
            .allocatorLayout(UILayoutMode::Vertical);

    // Side panel items
    const char* itemNames[] = {"Panel 1", "Panel 2", "Panel 3",
                               "Panel 4", "Panel 5"};
    for (const char* name : itemNames) {
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

    // --- Bottom panel: text with wrapping demo ---
    UIBuilder bottomPanel =
        uiWorld.child("Bottom Panel")
            .posAnchor(0.f, 0.f, 1.f, 0.2f)
            .allocatorFull()
            .rect(sf::Color(25, 25, 35, 210), sf::Color(80, 80, 100), 1.f);

    bottomPanel.child("Desc Text")
        .posFill()
        .text("This is the UI system demo for proj-animals. "
                 "Text automatically wraps within the BoundsComp area "
                 "when the wrap flag is enabled. Resize the window to "
                 "see the layout system reflow elements and the text "
                 "rewrap accordingly. Stencil clipping ensures content "
                 "stays within its panel boundaries.", "hack",
                 14, sf::Color(200, 200, 220), true);

    UIBuilder dragWindow =
        uiWorld.child("Draggable Window")
            .posAbsolute({{300.f, 400.f}, {250.f, 150.f}})
            .allocatorFull()
            .window(sf::Color(35, 35, 45, 240),
                sf::Color(130, 130, 150),
                sf::Color(70, 70, 100), 2.f, 24.f)
            .draggable(sf::FloatRect{{0.f, 150.f - 24.f}, {250.f, 24.f}})
            .zIndex(z_ui + 3);

    dragWindow.child("Draggable Text")
        .posAnchor(0.05f, 0.05f, 0.9f, 0.79f)
        .text("Drag me around by my header!\n\nPress ESC anytime to toggle my visibility.", "hack", 14, sf::Color::White, true);
}
