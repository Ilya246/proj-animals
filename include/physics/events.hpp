#pragma once
#include "entt/entt.hpp"
#include <SFML/Graphics/Rect.hpp>

struct GetDragEvent {
    float* drag;
};

struct BoundsResizeEvent {
    entt::registry* registry;
    entt::entity entity;
    sf::FloatRect newBounds;
};
