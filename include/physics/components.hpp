#pragma once
#include <entt/entt.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include "serialization/serialization.hpp"

struct PositionComp {
    sf::Vector2f position;
    entt::entity parent;

    REGISTER_SERIALIZABLE(PositionComp, Position)
};

struct PhysicsComp {
    // Move this much each second
    sf::Vector2f velocity;
    // Reduce velocity by this much each second
    float drag = 0.f;

    std::optional<sf::FloatRect> bounds{};

    REGISTER_SERIALIZABLE(PhysicsComp, Physics)
};

struct BoundsComp {
    sf::FloatRect bounds{{-16.f, -16.f}, {32.f, 32.f}};

    REGISTER_SERIALIZABLE(BoundsComp, Bounds)
};

template<typename TComp>
sf::FloatRect& get_optional_bounds(entt::entity fromEnt, TComp& from, entt::registry& reg) {
    if (from.bounds)
        return *from.bounds;
    return reg.get<BoundsComp>(fromEnt).bounds;
}
