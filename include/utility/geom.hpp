#pragma once
#include <array>
#include <SFML/Graphics/Rect.hpp>

struct DynamicBounds {
    sf::FloatRect bounds;
    std::array<bool, 4> isFraction; // TODO: serializable

    DynamicBounds(float pX, float pY, float sX, float sY, std::array<bool, 4> fractionMode);
};

// Applies adaptive bounds "bounds" onto base bounds "onto"
sf::FloatRect apply_dynamic_bounds(const sf::FloatRect& onto, const DynamicBounds& bounds);

sf::FloatRect rect_union(const sf::FloatRect& a, const sf::FloatRect& b);
