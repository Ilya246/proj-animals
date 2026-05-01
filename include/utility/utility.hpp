#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <functional>

const sf::Vector2f zeroVec{0.f, 0.f};

// Executes given function if APROJ_DEBUG is defined
// else, does nothing
void exec_debug(std::function<void()>&& fun);

struct DynamicBounds {
    sf::FloatRect bounds;
    std::array<bool, 4> isFraction; // TODO: serializable

    DynamicBounds(float pX, float pY, float sX, float sY, std::array<bool, 4> fractionMode);
};

// Applies adaptive bounds "bounds" onto base bounds "onto"
sf::FloatRect apply_dynamic_bounds(sf::FloatRect onto, DynamicBounds bounds);
