#pragma once
#include "entt/entt.hpp"
#include <SFML/Graphics/Rect.hpp>

struct GetDragEvent {
    float* drag;
};

struct BoundsResizeEvent {
    sf::FloatRect newBounds;
};
