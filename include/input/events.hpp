#pragma once
#include "entt/entt.hpp"
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>

struct GlobalClickEvent {
    // draw-coordinate pixel coordinates
    sf::Vector2i pixelCoords;
    sf::Mouse::Button button;
    entt::registry* registry;
};

struct ClickEvent {
    // draw-coordinate pixel coordinates
    sf::Vector2i pixelCoords;
    sf::Mouse::Button button;
    entt::entity ent;
    entt::registry* registry;
};

struct KeyPressEvent {
    sf::Keyboard::Key key;
    entt::registry* registry;
};
