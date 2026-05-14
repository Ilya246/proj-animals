#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <format>
#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include "editor/system.hpp"
#include "physics/system.hpp"
#include "utility/entity_builder.hpp"
#include "utility/geom.hpp"
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
    registry.ctx().emplace<uint64_t>(0);

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
        bool hadText = false;
        std::vector<KeyPressEvent> keyQ;
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
                keyQ.emplace_back(keyEv->code, &registry, nullptr);
                continue;
            }
            if (auto* textEv = event->getIf<sf::Event::TextEntered>()) {
                bool handled = false;
                GlobalTextEnteredEvent tEv{textEv->unicode, &registry, &handled};
                dispatcher.trigger(tEv);
                hadText = handled;
                continue;
            }
        }
        for (KeyPressEvent& e : keyQ) {
            bool handled = hadText;
            e.handled = &handled;
            dispatcher.trigger(e);
        }

        // Dispatch update event
        UpdateEvent updateEv(dt, &registry);
        dispatcher.trigger(updateEv);

        registry.ctx().get<uint64_t>()++;
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
    registry.emplace<PositionComp>(ball, pos).setParent(world, ball, registry);
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
    registry.emplace<PositionComp>(world, sf::Vector2f(0.f, 0.f)).setParent(world, world, registry);
    TileMapComp& mapComp = registry.emplace<TileMapComp>(world, 64, 64, 20.f, &tex_map["tileset_forest"]);
    registry.emplace<RenderableComp>(world, z_world);
    registry.emplace<BoundsComp>(world);
    // Add some random walls
    mapComp.grid.resize(mapComp.height * mapComp.width);
    for (TileType& t : mapComp.grid) t = {11, math::rand<uint8_t>(4, 7)};
    for (int i = 0; i < 200; ++i) {
        int x = math::rand(0, mapComp.width - 1);
        int y = math::rand(0, mapComp.height - 1);

        sf::Sprite wallSprite(tex_map["wall"]);
        wallSprite.setOrigin(sf::Vector2f(tex_map["wall"].getSize()) / 2.f);

        EntityBuilder{registry, "Wall " + std::to_string(i)}
            .pos(x * 32.f + 16.f, y * 32.f + 16.f, world)
            .bounds(-16.f, -16.f, 32.f, 32.f)
            .collider(CollisionLayer::Wall, CollisionLayer::None)
            .sprite(std::move(wallSprite));
    }
    // Generate the efficient vertex array
    MapUtil::rebuildMesh(world, mapComp, registry);

    // Create a player entity
    entt::entity player = registry.create();
    registry.emplace<PositionComp>(player, sf::Vector2f(0.f, 0.f)).setParent(world, player, registry);
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
    registry.emplace<PositionComp>(camera, sf::Vector2f(0.f, 0.f)).setParent(player, camera, registry);
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
        .posAbsolute({{200.f, 200.f}, {200.f, 300.f}})
        .draggable()
        .rect(sf::Color(30, 30, 50, 100), sf::Color(170, 170, 200, 120), 2.f)
        .allocatorLayout(UILayoutMode::Vertical, 2.f, 2.f, DynamicBounds::full, true)
        .emplace<EditorComp>()
        .buttonToggled(sf::Keyboard::Key::F3);

    UIBuilder compPanel = editorContainer.child("Component Panel")
        .posFill()
        .allocatorLayout(UILayoutMode::Vertical, 0.f, 0.f)
        .emplace<ComponentEditorUIComp>()
        .hide();

    // Construct the parameter box container list
    entt::entity paramsContainer = compPanel.child("Component Parameters")
        .posFill()
        .allocatorLayout(UILayoutMode::Vertical, 0.f, 1.f)
        .get();

    registry.get<ComponentEditorUIComp>(compPanel).paramListContainer = paramsContainer;

    compPanel.child("Component Dropdown")
        .posFill()
        .rect(sf::Color(60, 60, 80), sf::Color(100, 100, 120), 1.f)
        .constraint(0.f, 24.f, true, false)
        .childText("Select Component...", "hack", 12)
        .dropdownTrigger([](entt::registry& reg, entt::entity world, entt::entity trigger) {
            entt::entity selectedEnt = entt::null;
            for (auto [e, esc] : reg.view<EditorSelectedComp>().each()) { selectedEnt = e; break; }

            sf::FloatRect triggerBounds = reg.get<BoundsComp>(trigger).bounds;
            sf::Vector2f worldPos = Physics::getWorldPos(trigger, reg);

            // Shift the list visually below the dropdown trigger.
            sf::FloatRect listBounds{{worldPos.x, worldPos.y - 100.f}, {triggerBounds.size.x, 100.f}};

            UIBuilder list = UIBuilder(reg, world, "Dropdown List")
                .posAbsolute(listBounds)
                .zIndex(z_ui + 1024)
                .rect(sf::Color(40, 40, 40, 240), sf::Color(150, 150, 150, 200), 1.f)
                .scrollable(DynamicBounds::full, sf::Color(30, 30, 30, 200), sf::Color(100, 100, 100), sf::Color(150, 150, 150), 1.f)
                .allocatorLayout(UILayoutMode::Vertical, 2.f, 2.f, DynamicBounds::full, false)
                .stencil()
                .ensure<HoverListenerComp>(); // Required to intercept Hover checks and not instantly auto-close

            auto& bounds = reg.get<BoundsComp>(list.get());
            bounds.resize({{0.f, 0.f}, listBounds.size}, list.get(), reg);

            if (selectedEnt != entt::null) {
                bool hasComps = false;
                for (const auto& name : ComponentSerializer::get_registered_types()) {
                    if (ComponentSerializer::serialize(name, reg, selectedEnt)) {
                        hasComps = true;
                        list.child("DD Item " + name)
                            .posFill()
                            .constraint(0.f, 20.f, true, false)
                            .rect(sf::Color(60, 60, 60), sf::Color::Transparent, 0.f)
                            .button([name, listEnt = list.get()](ClickEvent& ev, entt::entity, entt::registry& r) {
                                for(auto [e, ce] : r.view<ComponentEditorUIComp>().each()) {
                                    ce.selectedComponent = name;
                                }
                                queue_delete(listEnt, r); // Selecting closes list immediately
                                ev.handled = true;
                            })
                            .childText(name, "hack", 12);
                    }
                }
                if (!hasComps) {
                    list.child("DD Item Empty")
                        .posFill()
                        .constraint(0.f, 20.f, true, false)
                        .childText("No Components Found", "hack", 12);
                }
            } else {
                 list.child("DD Item None")
                    .posFill()
                    .constraint(0.f, 20.f, true, false)
                    .childText("No Entity Selected", "hack", 12);
            }

            return list.get();
        }, false); // don't open on hover by default

    UIBuilder tilePanel = editorContainer.child("Tile Panel")
        .posFill()
        .childText(std::string(60, 'b'), "hack", 12)
        .constraint(0.f, 32.f, true, true)
        .hide();

    UIBuilder selectorPanel = editorContainer.child("Selector Panel")
        .posFill()
        .rect(sf::Color(50, 50, 50, 200), sf::Color(100, 100, 100), 1.f)
        .allocatorLayout(UILayoutMode::Horizontal, 2.f, 2.f, DynamicBounds::full);

    entt::entity tbComp = selectorPanel.child("Comp Editor TB")
        .posFill()
        .rect(sf::Color(80, 80, 80), sf::Color(120, 120, 120), 1.f)
        .toggleButton(false, sf::Color(100, 150, 100), sf::Color(80, 80, 80),
        [ent = compPanel.get()] (bool on, entt::entity self, entt::registry& reg) {
            reg.get<UIComp>(ent).set_hidden(!on, reg, self);
        })
        .constraint(0.f, 30.f, true, false)
        .childText("Comp", "hack", 12)
        .get();

    entt::entity tbTile = selectorPanel.child("Tile Editor TB")
        .posFill()
        .rect(sf::Color(80, 80, 80), sf::Color(120, 120, 120), 1.f)
        .toggleButton(false, sf::Color(100, 150, 100), sf::Color(80, 80, 80),
        [ent = tilePanel.get()](bool on, entt::entity self, entt::registry& reg) {
            reg.get<UIComp>(ent).set_hidden(!on, reg, self);
        })
        .constraint(0.f, 30.f, true, false)
        .text("Tile", "hack", 12)
        .get();

    registry.get<ToggleButtonComp>(tbComp).exclusiveGroup.push_back(tbTile);
    registry.get<ToggleButtonComp>(tbTile).exclusiveGroup.push_back(tbComp);

    UIBuilder topPanel = editorContainer.child("Top Panel")
        .posFill()
        .rect(sf::Color(40, 40, 40, 200), sf::Color(100, 100, 100), 1.f)
        .allocatorLayout(UILayoutMode::Horizontal, 2.f, 2.f, DynamicBounds::full);

    auto makeModeButton = [&](const std::string& text, EditorMode mode) {
        return topPanel.child(text + " Button")
            .posFill()
            .rect(sf::Color(80, 80, 100, 200), sf::Color(120, 120, 140), 1.f)
             .toggleButton(mode == EditorMode::None, sf::Color(100, 100, 150), sf::Color(80, 80, 100),
             [mode](bool on, entt::entity, entt::registry& reg) {
                auto& sys = reg.ctx().get<EditorSystem>();
                if (on) sys.mode = mode;
                else if (sys.mode == mode) sys.mode = EditorMode::None;
            })
            .childText(text, "hack", 14)
            .constraint(0.f, 30.f, true, false)
            .tooltip("This is the " + text + " mode button.");
    };

    entt::entity btnSelect = makeModeButton("[]", EditorMode::Select);

    topPanel.child("Delete Button")
        .posFill()
        .rect(sf::Color(100, 60, 60, 200), sf::Color(140, 120, 120), 1.f)
        .button([](ClickEvent& ev, entt::entity, entt::registry& reg) {
            auto view = reg.view<EditorSelectedComp>();
            for (auto ent : view) {
                queue_delete(ent, reg);
            }
            ev.handled = true;
        })
        .childText("X", "hack", 14)
        .constraint(0.f, 30.f, true, false)
        .tooltip("Deletes the selected entity.");

    entt::entity btnSpawn = makeModeButton("+", EditorMode::Spawn);
    entt::entity btnNone = makeModeButton("M", EditorMode::None);

    registry.get<ToggleButtonComp>(btnSelect).exclusiveGroup = {btnSpawn, btnNone};
    registry.get<ToggleButtonComp>(btnSpawn).exclusiveGroup = {btnSelect, btnNone};
    registry.get<ToggleButtonComp>(btnNone).exclusiveGroup = {btnSelect, btnSpawn};

    editorContainer.hide();
}
