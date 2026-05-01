#pragma once
#include <string>
#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>

#include "core/components.hpp"
#include "graphics/components.hpp"
#include "physics/components.hpp"
#include "input/components.hpp"
#include "serialization/serialization.hpp"
#include "utility/constants.hpp"

struct EntityBuilder{
    entt::registry& reg;
    entt::entity ent;
    EntityBuilder(entt::entity ent, entt::registry& r) : reg(r), ent(ent) {};

    EntityBuilder(entt::registry& r, const std::string& name = "") : reg(r), ent(r.create())
    {
        if (!name.empty()) set_ent_name(ent, r, name);
        r.emplace<PositionComp>(ent, zeroVec, entt::null);
        r.emplace<RenderableComp>(ent, z_entity);
        r.emplace<BoundsComp>(ent);
    }

    entt::entity get() const { return ent; }
    operator entt::entity() const { return ent; }

    template<typename Comp, typename... Args>
    EntityBuilder& emplace(Args&&... args) {
        reg.emplace<Comp>(ent, std::forward<Args>(args)...);
        return *this;
    }

    template<typename Comp, typename... Args>
    EntityBuilder& ensure(Args&&... args) {
        if (!reg.try_get<Comp>(ent))
            reg.emplace<Comp>(ent, std::forward<Args>(args)...);
        return *this;
    }

    template<typename Comp, typename... Args>
    EntityBuilder& ensure_force(Args&&... args) {
        reg.emplace_or_replace<Comp>(ent, std::forward<Args>(args)...);
        return *this;
    }

    EntityBuilder& name(std::string_view n) {
        set_ent_name(ent, reg, n);
        return *this;
    }

    EntityBuilder& pos(float x, float y, entt::entity parent = entt::null) {
        auto& p = reg.get<PositionComp>(ent);
        p.position = {x, y};
        p.parent = parent;
        return *this;
    }

    EntityBuilder& pos(sf::Vector2f v, entt::entity parent = entt::null) {
        return pos(v.x, v.y, parent);
    }

    EntityBuilder& zLayer(ZLevel z) {
        reg.get<RenderableComp>(ent).zLevel = static_cast<int32_t>(z);
        return *this;
    }

    EntityBuilder& bounds(sf::FloatRect rect) {
        reg.get<BoundsComp>(ent).bounds = rect;
        return *this;
    }

    EntityBuilder& sprite(sf::Sprite&& sprite) {
        ensure_force<SpriteComp>(sprite);
        return *this;
    }

    EntityBuilder& camera(float scale = 1.f, ZLevel z = z_world) {
        return emplace<CameraComp>(scale, static_cast<int32_t>(z));
    }

    EntityBuilder& collider(CollisionLayer layer = CollisionLayer::Wall | CollisionLayer::Player, CollisionLayer mask = CollisionLayer::Player, float mass = 5.f, float restitution = 0.5f) {
        return emplace<ColliderComp>(layer, mask, mass, restitution);
    }

    EntityBuilder& movable(float speed, float accel) {
        return emplace<InputMovementComp>(speed, accel);
    }

    EntityBuilder& nonSerializable() {
        return emplace<NonSerializableComp>();
    }
};
