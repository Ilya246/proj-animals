#include <SFML/System/Vector2.hpp>
#include <entt/entt.hpp>

#include "core/events.hpp"
#include "entt/entity/fwd.hpp"
#include "physics/components.hpp"
#include "utility/utility.hpp"
#include "world/components.hpp"
#include "physics/system.hpp"

REGISTER_SYSTEM(PhysicsSystem);

void PhysicsSystem::init(entt::registry& reg) {
    entt::dispatcher& dispatch = ev_dispatcher(reg);

    dispatch.sink<UpdateEvent>().connect<&PhysicsSystem::update>(this);
}

void PhysicsSystem::update(const UpdateEvent& ev) {
    entt::registry& reg = *ev.registry;

    auto view = ev.registry->view<PositionComp, PhysicsComp>();
    for (auto [entity, pos, vel] : view.each()) {
        // Update position
        sf::Vector2f newPos = pos.position + vel.velocity * ev.dt;
        auto [world, newWorldPos] = Physics::getWorldPos(pos.parent, newPos, reg);

        // Check tile collision
        if (TileMapComp* mapComp = reg.try_get<TileMapComp>(world)) {
            if (vel.velocity != zeroVec && MapUtil::getTileAt(newWorldPos, *mapComp) == TileType::Wall) {
                float tileSize = mapComp->tileSize;
                sf::Vector2i intNewPos = MapUtil::getTilePos(newPos, tileSize);
                sf::Vector2f tilePos = (sf::Vector2f)intNewPos * tileSize;
                sf::Vector2f relTile = tilePos - pos.position;
                sf::Vector2f normVel = vel.velocity.normalized();
                float int1 = relTile.dot(normVel);
                float int2 = int1 + normVel.x * tileSize; // dot if we add +1 to tilePos.x
                float int3 = int1 + normVel.y * tileSize; // dot if we add +1 to tilePos.y
                float int4 = int1 + (normVel.x + normVel.y) * tileSize; // if both
                float intMin = std::min(std::min(int1, int2), std::min(int3, int4));
                sf::Vector2f intP = relTile - normVel * intMin;
                float halfTile = tileSize * 0.5f;
                if (std::abs(intP.y - halfTile) >= halfTile) vel.velocity.y *= 0.f;
                if (std::abs(intP.x - halfTile) >= halfTile) vel.velocity.x *= 0.f;
                newPos = pos.position + vel.velocity * ev.dt;
            }
        }

        pos.position = newPos;

        // Update velocity
        if (vel.velocity != zeroVec) {
            float dragBy = vel.drag * ev.dt;
            if (vel.velocity.lengthSquared() <= dragBy)
                vel.velocity = zeroVec;
            else
                vel.velocity -= vel.velocity.normalized() * dragBy;
        }
    }
}

namespace Physics {

entt::entity getWorld(const entt::entity& ent, const entt::registry& reg) {
    const PositionComp& comp = reg.get<PositionComp>(ent);
    if (comp.parent == ent)
        return ent;
    return getWorld(comp.parent, reg);
}

sf::Vector2f worldPos(const entt::entity& ent, const entt::registry& reg) {
    const PositionComp& comp = reg.get<PositionComp>(ent);
    if (comp.parent == ent)
        return comp.position;
    return worldPos(comp.parent, reg) + comp.position;
}

std::pair<entt::entity, sf::Vector2f> getWorldPos(const entt::entity& ent, const entt::registry& reg) {
    const PositionComp& comp = reg.get<PositionComp>(ent);
    if (comp.parent == ent)
        return {ent, comp.position};
    auto lower = getWorldPos(comp.parent, reg);
    return {lower.first, lower.second + comp.position};
}

std::pair<entt::entity, sf::Vector2f> getWorldPos(const entt::entity& parent, const sf::Vector2f& pos, const entt::registry& reg) {
    auto lower = getWorldPos(parent, reg);
    return {lower.first, lower.second + pos};
}

}