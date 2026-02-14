#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <entt/entity/fwd.hpp>
#include <SFML/System/Vector2.hpp>

struct PositionComp {
    sf::Vector2f position;
    entt::entity parent;
};

struct PhysicsComp {
    // Move this much each second
    sf::Vector2f velocity;
    // Reduce velocity by this much each second
    float drag = 0.f;

    sf::FloatRect bounds{{-16.f, -16.f}, {32.f, 32.f}};
};