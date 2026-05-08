#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <format>
#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include "editor/system.hpp"
#include "yaml-cpp/node/parse.h"

#include "core/events.hpp"
#include "core/system.hpp"
#include "core/entity.hpp"
#include "editor/components.hpp"
#include "graphics/components.hpp"
#include "graphics/texture.hpp"
#include "input/components.hpp"
#include "input/events.hpp"
#include "physics/components.hpp"
#include "serialization/serialization.hpp"
#include "ui/builder.hpp"
#include "ui/components.hpp"
#include "utility/math.hpp"
#include "world/components.hpp"

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

    std::vector<std::unique_ptr<SystemBase>> systems = create_systems(registry);

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
                bool handled = false;
                GlobalClickEvent clickEv{mouseEv->position, mouseEv->button, true, &registry, &handled};
                dispatcher.trigger(clickEv);
                continue;
            }
            if (auto* mouseEv = event->getIf<sf::Event::MouseButtonReleased>()) {
                bool handled = false;
                GlobalClickEvent clickEv{mouseEv->position, mouseEv->button, false, &registry, &handled};
                dispatcher.trigger(clickEv);
                continue;
            }
            if (auto* wheelEv = event->getIf<sf::Event::MouseWheelScrolled>()) {
                bool handled = false;
                GlobalScrollEvent scrollEv{wheelEv->position, wheelEv->delta, &registry, &handled};
                dispatcher.trigger(scrollEv);
                continue;
            }
            if (auto* moveEv = event->getIf<sf::Event::MouseMoved>()) {
                GlobalMouseMoveEvent mouseEv{moveEv->position, &registry};
                dispatcher.trigger(mouseEv);
                continue;
            }
            if (auto* resizeEv = event->getIf<sf::Event::Resized>()) {
                ScreenResizeEvent screenEv{resizeEv->size.x, resizeEv->size.y, &registry};
                dispatcher.trigger(screenEv);
                continue;
            }
            if (auto* keyEv = event->getIf<sf::Event::KeyPressed>()) {
                bool handled = false;
                KeyPressEvent pressEv{keyEv->code, &registry, &handled};
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
    registry.emplace<ButtonComp>(ball, [](ClickEvent&, entt::entity, entt::registry&) { std::cout << "test" << std::endl; });
    registry.emplace<ColliderComp>(ball, CollisionLayer::Player, CollisionLayer::Wall | CollisionLayer::Player, 5.f * size_m * size_m);

    sf::Sprite ballSprite(tex_map["mob"]);
    ballSprite.setColor(sf::Color::Red);        // tint red
    ballSprite.setOrigin(sf::Vector2f(tex_map["mob"].getSize()) / 2.f);
    // Scale down a bit so balls are smaller
    ballSprite.setScale({size_m, size_m});
    registry.emplace<SpriteComp>(ball, ballSprite);
    registry.emplace<TextComp>(ball, sf::Text(font_map["hack"], "test", 15));
    registry.emplace<RenderableComp>(ball, z_entity);
}

void genWorld(entt::registry& registry) {
    entt::entity world = registry.create();
    registry.emplace<PositionComp>(world, sf::Vector2f(0.f, 0.f), world);
    TileMapComp& mapComp = registry.emplace<TileMapComp>(world, 64, 64, 20.f, &tex_map["notdrnottileset"]);
    registry.emplace<RenderableComp>(world, z_world);
    registry.emplace<BoundsComp>(world);
    // Add some random walls
    mapComp.grid.resize(mapComp.height * mapComp.width);
    for (TileType& t : mapComp.grid) t = {math::rand<uint8_t>(3, 9), 3};
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
        registry.emplace<SpriteComp>(wall, wallSprite);
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
    registry.emplace<SpriteComp>(player, playerSprite);
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

    // Prototype Editor UI replacing main.cpp genUI
    UIBuilder uiWorld = UIBuilder::makeWorld(registry, "UI World");

    UIBuilder editorContainer = uiWorld.child("Editor Container")
        .posAbsolute({{200.f, 200.f}, {150.f, 300.f}})
        .draggable()
        .rect(sf::Color(30, 30, 50, 100), sf::Color(170, 170, 200, 120), 2.f)
        .allocatorFull()
        .emplace<EditorComp>()
        .buttonToggled(sf::Keyboard::Key::F3);

    UIBuilder topPanel = editorContainer.child("Top Panel")
        .posAnchor(0.f, 260.f, 1.f, 40.f, {true, false, true, false})
        .rect(sf::Color(40, 40, 40, 200), sf::Color(100, 100, 100), 1.f)
        .allocatorLayout(UILayoutMode::Horizontal, 2.f, 2.f, DynamicBounds::full);

    auto makeModeButton = [&](const std::string& text, EditorMode mode) {
        topPanel.child(text + " Button")
            .posFill()
            .rect(sf::Color(80, 80, 100, 200), sf::Color(120, 120, 140), 1.f)
            .button([mode](ClickEvent& ev, entt::entity, entt::registry& reg) {
                auto& sys = reg.ctx().get<EditorSystem&>();
                sys.mode = sys.mode == mode ? EditorMode::None : mode;
                *ev.handled = true;
            })
            .text(text, "hack", 14);
    };

    makeModeButton("Select", EditorMode::Select);
    
    topPanel.child("Delete Button")
        .posFill()
        .rect(sf::Color(100, 60, 60, 200), sf::Color(140, 120, 120), 1.f)
        .button([](ClickEvent& ev, entt::entity, entt::registry& reg) {
            auto view = reg.view<EditorSelectedComp>();
            for (auto ent : view) {
                queue_delete(ent, reg);
            }
            *ev.handled = true;
        })
        .text("Delete", "hack", 14);

    makeModeButton("Spawn", EditorMode::Spawn);
    makeModeButton("Add Comp", EditorMode::AddComp);

    editorContainer.hide();
}
