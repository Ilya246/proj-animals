#pragma once
#include "entt/entt.hpp"
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>

struct GlobalClickEvent {
    // draw-coordinate pixel coordinates
    sf::Vector2i pixelCoords;
    sf::Mouse::Button button;
    bool pressed;
    entt::registry* registry;
    bool* handled;
};

struct ClickEvent {
    // draw-coordinate pixel coordinates
    sf::Vector2i pixelCoords;
    sf::Vector2f relativeCoords;
    sf::Vector2f worldCoords;
    sf::Mouse::Button button;
    bool pressed;
    bool handled;
};

struct GlobalScrollEvent {
    sf::Vector2i pixelCoords;
    float delta;
    entt::registry* registry;
    bool* handled;
};

struct ScrollEvent {
    sf::Vector2i pixelCoords;
    sf::Vector2f relativeCoords;
    sf::Vector2f worldCoords;
    float delta;
    bool handled;
};

struct GlobalMouseMoveEvent {
    sf::Vector2i pixelCoords;
    entt::registry* registry;
};

struct KeyPressEvent {
    sf::Keyboard::Key key;
    entt::registry* registry;
    bool* handled;
};

struct InputMovedEvent {
    sf::Vector2f input;
};

struct GlobalHoverEvent {
    sf::Vector2i pixelCoords;
    entt::registry* registry;
    bool* handled;
};

struct HoverEvent {
    sf::Vector2i pixelCoords;
    sf::Vector2f relativeCoords;
    sf::Vector2f worldCoords;
    float dt;
    uint64_t tick;
    bool& handled;
};
