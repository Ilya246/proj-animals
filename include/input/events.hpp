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
};

struct ClickEvent {
    // draw-coordinate pixel coordinates
    sf::Vector2i pixelCoords;
    sf::Vector2f relativeCoords;
    sf::Vector2f worldCoords;
    sf::Mouse::Button button;
    entt::entity ent;
    bool pressed;
    entt::registry* registry;
    bool* handled;
};

struct GlobalScrollEvent {
    sf::Vector2i pixelCoords;
    float delta;
    entt::registry* registry;
};

struct ScrollEvent {
    sf::Vector2i pixelCoords;
    sf::Vector2f relativeCoords;
    sf::Vector2f worldCoords;
    float delta;
    entt::entity ent;
    entt::registry* registry;
    bool* handled;
};

struct GlobalMouseMoveEvent {
    sf::Vector2i pixelCoords;
    entt::registry* registry;
};

struct KeyPressEvent {
    sf::Keyboard::Key key;
    entt::registry* registry;
};

struct InputMovedEvent {
    sf::Vector2f input;
    entt::entity ent;
    entt::registry* reg;
};
