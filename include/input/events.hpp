#pragma once
#include "entt/entt.hpp"
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>

struct ClickEvent {
    // draw-coordinate pixel coordinates
    sf::Vector2i pixelCoords;
    sf::Mouse::Button button;
    entt::registry* registry;
};
