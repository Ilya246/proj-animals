#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstdlib>
#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <filesystem>
#include <iostream>

#include "core/components.hpp"
#include "core/events.hpp"
#include "core/system.hpp"
#include "graphics/components.hpp"
#include "graphics/texture.hpp"
#include "input/components.hpp"
#include "input/events.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "serialization/serialization.hpp"
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
                ClickEvent clickEv(mouseEv->position, mouseEv->button, &registry);
                dispatcher.trigger(clickEv);
                continue;
            }
            if (auto* resizeEv = event->getIf<sf::Event::Resized>()) {
                ScreenResizeEvent screenEv{resizeEv->size.x, resizeEv->size.y, &registry};
                dispatcher.trigger(screenEv);
                continue;
            }
        }

        // Dispatch update event
        UpdateEvent updateEv(dt, &registry);
        dispatcher.trigger(updateEv);
    }

    std::cout << "Writing save to save.yml...";
    std::ofstream save_s("save.yml");
    save_s << serialize_registry(registry);

    return 0;
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
        const auto ball = registry.create();
        sf::Vector2f pos(math::randf(50.f, 750.f), math::randf(50.f, 550.f));
        sf::Vector2f vel(math::randf(-200.f, 200.f), math::randf(-200.f, 200.f));
        registry.emplace<PositionComp>(ball, pos, world);
        registry.emplace<PhysicsComp>(ball, vel, 10.f);
        registry.emplace<BoundsComp>(ball, sf::FloatRect{{-8.f, -8.f}, {16.f, 16.f}});
        registry.emplace<ClickListenerComp>(ball);
        registry.emplace<ButtonComp>(ball, [](ClickEvent&) { std::cout << "test" << std::endl; });
        registry.emplace<ColliderComp>(ball, CollisionLayer::Player, CollisionLayer::Wall | CollisionLayer::Player, 2.f);

        sf::Sprite ballSprite(tex_map["mob"]);
        ballSprite.setColor(sf::Color::Red);        // tint red
        ballSprite.setOrigin(sf::Vector2f(tex_map["mob"].getSize()) / 2.f);
        // Scale down a bit so balls are smaller
        ballSprite.setScale({0.5f, 0.5f});
        registry.emplace<SpriteComp>(ball, std::move(ballSprite));
        registry.emplace<TextComp>(ball, sf::Text(font_map["hack"], "test", 15));
        registry.emplace<RenderableComp>(ball, z_entity);
    }
}

void genUI(entt::registry& registry) {
    // --- UI world root entity ---
    entt::entity uiWorld = registry.create();
    set_ent_name(uiWorld, registry, "UI World");
    registry.emplace<PositionComp>(uiWorld, sf::Vector2f(0.f, 0.f), uiWorld);
    registry.emplace<BoundsComp>(uiWorld, sf::FloatRect{{0.f, 0.f}, {800.f, 600.f}});
    UIFullAllocatorComp& worldAlloc = registry.emplace<UIFullAllocatorComp>(uiWorld);

    // --- UI camera (will be repositioned by UIScreenComp on resize) ---
    entt::entity uiCamera = registry.create();
    set_ent_name(uiCamera, registry, "UI Camera");
    registry.emplace<PositionComp>(uiCamera, sf::Vector2f(0.f, 0.f), uiWorld);
    registry.emplace<CameraComp>(uiCamera, 1.f, z_ui);

    // Link screen component to camera
    registry.emplace<UIScreenComp>(uiWorld, uiCamera);
    registry.emplace<RenderableComp>(uiWorld, z_ui);

    // --- Top panel: a bordered panel at the top of the screen ---
    entt::entity topPanel = registry.create();
    set_ent_name(topPanel, registry, "Top Panel");
    registry.emplace<PositionComp>(topPanel, sf::Vector2f(0.f, 0.f), uiWorld);
    registry.emplace<UIAnchorComp>(topPanel, sf::FloatRect{{0.f, 0.7f}, {1.f, 0.3f}});
    worldAlloc.children.push_back(topPanel);
    registry.emplace<BoundsComp>(topPanel, sf::FloatRect{{0.f, 0.f}, {800.f, 100.f}});
    registry.emplace<RenderableComp>(topPanel, z_ui);
    registry.emplace<UIRectComp>(topPanel, sf::Color(30, 30, 30, 200),
                                  sf::Color::White, 2.f);
    registry.emplace<StencilDrawComp>(topPanel);
    auto& topLayout = registry.emplace<UILayoutComp>(topPanel);
    topLayout.mode = UILayoutMode::Horizontal;
    topLayout.padding = 8.f;
    topLayout.spacing = 8.f;

    // --- Child: title text ---
    entt::entity titleText = registry.create();
    set_ent_name(titleText, registry, "Title Text");
    registry.emplace<PositionComp>(titleText, sf::Vector2f(0.f, 0.f), topPanel);
    registry.emplace<UIFillComp>(titleText);
    registry.emplace<BoundsComp>(titleText, sf::FloatRect{{0.f, 0.f}, {200.f, 40.f}});
    registry.emplace<RenderableComp>(titleText, z_ui + 1);
    auto& titleTextComp = registry.emplace<TextComp>(titleText,
        sf::Text(font_map["hack"], "Animals UI Demo", 22));
    titleTextComp.text.setFillColor(sf::Color::White);
    titleTextComp.wrap = false;

    // --- Child: spacer (fills remaining horizontal space) ---
    entt::entity spacer = registry.create();
    set_ent_name(spacer, registry, "Spacer");
    registry.emplace<PositionComp>(spacer, sf::Vector2f(0.f, 0.f), topPanel);
    registry.emplace<UIFillComp>(spacer);
    registry.emplace<BoundsComp>(spacer, sf::FloatRect{{0.f, 0.f}, {1.f, 1.f}});
    registry.emplace<RenderableComp>(spacer, z_ui + 1);

    // --- Child: status text ---
    entt::entity statusText = registry.create();
    set_ent_name(statusText, registry, "Status Text");
    registry.emplace<PositionComp>(statusText, sf::Vector2f(0.f, 0.f), topPanel);
    registry.emplace<UIFillComp>(statusText);
    registry.emplace<BoundsComp>(statusText, sf::FloatRect{{0.f, 0.f}, {150.f, 40.f}});
    registry.emplace<RenderableComp>(statusText, z_ui + 1);
    auto& statusTextComp = registry.emplace<TextComp>(statusText,
        sf::Text(font_map["hack"], "Status: OK", 16));
    statusTextComp.text.setFillColor(sf::Color::Green);

    topLayout.children = {titleText, spacer, statusText};

    // --- Side panel: vertical list on the left ---
    entt::entity sidePanel = registry.create();
    set_ent_name(sidePanel, registry, "Side Panel");
    registry.emplace<PositionComp>(sidePanel, sf::Vector2f(0.f, 0.f), uiWorld);
    registry.emplace<UIAnchorComp>(sidePanel, sf::FloatRect{{0.f, 0.2f}, {0.2f, 0.5f}});
    worldAlloc.children.push_back(sidePanel);
    registry.emplace<BoundsComp>(sidePanel, sf::FloatRect{{0.f, 0.f}, {180.f, 500.f}});
    registry.emplace<RenderableComp>(sidePanel, z_ui);
    registry.emplace<UIRectComp>(sidePanel, sf::Color(40, 40, 50, 220),
                                  sf::Color(100, 100, 120), 1.f);
    registry.emplace<StencilDrawComp>(sidePanel);
    auto& sideLayout = registry.emplace<UILayoutComp>(sidePanel);
    sideLayout.mode = UILayoutMode::Vertical;

    // Side panel items
    const char* itemNames[] = {"Panel 1", "Panel 2", "Panel 3",
                               "Panel 4", "Panel 5"};

    for (const char* name : itemNames) {
        entt::entity item = registry.create();
        set_ent_name(item, registry, name);
        registry.emplace<PositionComp>(item, sf::Vector2f(0.f, 0.f), sidePanel);
        registry.emplace<UIFillComp>(item);
        registry.emplace<BoundsComp>(item, sf::FloatRect{{0.f, 0.f}, {168.f, 28.f}});
        registry.emplace<RenderableComp>(item, z_ui + 1);
        registry.emplace<UIRectComp>(item, sf::Color(0, 0, 0, 0),
                                      sf::Color(120, 120, 140), 1.f);
        auto& itemTextComp = registry.emplace<TextComp>(item,
            sf::Text(font_map["hack"], name, 14));
        itemTextComp.text.setFillColor(sf::Color::White);
        sideLayout.children.push_back(item);
    }

    // --- Bottom panel: text with wrapping demo ---
    entt::entity bottomPanel = registry.create();
    set_ent_name(bottomPanel, registry, "Bottom Panel");
    registry.emplace<PositionComp>(bottomPanel, sf::Vector2f(0.f, 0.f), uiWorld);
    registry.emplace<UIAnchorComp>(bottomPanel, sf::FloatRect{{0.f, 0.f}, {1.f, 0.2f}});
    worldAlloc.children.push_back(bottomPanel);
    auto& bottomAlloc = registry.emplace<UIFullAllocatorComp>(bottomPanel);
    registry.emplace<BoundsComp>(bottomPanel, sf::FloatRect{{0.f, 0.f}, {620.f, 120.f}});
    registry.emplace<RenderableComp>(bottomPanel, z_ui);
    registry.emplace<UIRectComp>(bottomPanel, sf::Color(25, 25, 35, 210),
                                  sf::Color(80, 80, 100), 1.f);
    registry.emplace<StencilDrawComp>(bottomPanel);

    entt::entity descText = registry.create();
    set_ent_name(descText, registry, "Desc Text");
    registry.emplace<PositionComp>(descText, sf::Vector2f(0.f, 0.f), bottomPanel);
    registry.emplace<UIFillComp>(descText);
    registry.emplace<BoundsComp>(descText, sf::FloatRect{{0.f, 0.f}, {616.f, 116.f}});
    registry.emplace<RenderableComp>(descText, z_ui + 1);
    auto& descTextComp = registry.emplace<TextComp>(descText,
        sf::Text(font_map["hack"],
                 "This is the UI system demo for proj-animals. "
                 "Text automatically wraps within the BoundsComp area "
                 "when the wrap flag is enabled. Resize the window to "
                 "see the layout system reflow elements and the text "
                 "rewrap accordingly. Stencil clipping ensures content "
                 "stays within its panel boundaries.",
                 14));
    descTextComp.text.setFillColor(sf::Color(200, 200, 220));
    bottomAlloc.children.push_back(descText);

    auto rend_view = registry.view<RenderableComp, PositionComp>();
    for (auto [entity, rend, pos] : rend_view.each()) {
        if (Physics::getWorld(entity, registry) == uiWorld)
            registry.emplace<NonSerializableComp>(entity);
    }
}
