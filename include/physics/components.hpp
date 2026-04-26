#pragma once
#include <entt/entt.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <unordered_map>
#include "entt/entity/fwd.hpp"
#include "serialization/serialization.hpp"

struct PositionComp {
    sf::Vector2f position;
    entt::entity parent;

    REGISTER_SERIALIZABLE(PositionComp, Position)
};

struct PhysicsComp {
    // m/s velocity
    sf::Vector2f velocity = zeroVec;
    // m/s^2 drag
    float drag = 0.f;

    std::optional<sf::FloatRect> bounds{};

    REGISTER_SERIALIZABLE(PhysicsComp, Physics)
};

struct BoundsComp {
    sf::FloatRect bounds{{-16.f, -16.f}, {32.f, 32.f}};

    // Resize while firing BoundsResizeEvent
    void resize(const sf::FloatRect& newBounds, entt::entity e, entt::registry& reg);

    REGISTER_SERIALIZABLE(BoundsComp, Bounds)
};

enum class CollisionLayer : uint16_t {
    None = 0,
    Player = 1 << 0,
    Wall = 1 << 1,
};

inline CollisionLayer operator|(CollisionLayer a, CollisionLayer b) {
    return (CollisionLayer)((uint16_t)a | (uint16_t)b);
}

inline CollisionLayer operator&(CollisionLayer a, CollisionLayer b) {
    return (CollisionLayer)((uint16_t)a & (uint16_t)b);
}

// Lets this entity collide with other ColliderComp entities
struct ColliderComp {
    CollisionLayer layer = CollisionLayer::None;
    CollisionLayer mask = CollisionLayer::None;
    float mass = 5.f;
    float restitution = 0.5f;

    // used in the system to block movement of freshly collided ents
    bool move_blocked = false;

    REGISTER_SERIALIZABLE(ColliderComp, Collider)
};

// Required for child entities to have collision
// Does *not* currently support cross-parent collision
struct ColliderMapComp {
    float chunk_size = 32.f;
    std::unordered_map<uint64_t, std::vector<entt::entity>> spatial_map = {};

    std::pair<int32_t, int32_t> to_chunk(sf::Vector2f pos);
    uint64_t to_hash(sf::Vector2f pos);
    uint64_t chunk_hash(int32_t x, int32_t y);
};

template<typename TComp>
sf::FloatRect& get_optional_bounds(entt::entity fromEnt, TComp& from, entt::registry& reg) {
    if (from.bounds)
        return *from.bounds;
    return reg.get<BoundsComp>(fromEnt).bounds;
}
