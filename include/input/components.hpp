#pragma once
#include <SFML/System/Vector2.hpp>

struct InputMovementComp {
    float speed;
    float accel;

    sf::Vector2f lastInput{0.f, 0.f};
};