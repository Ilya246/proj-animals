#include <SFML/System/Vector2.hpp>
#include <entt/entt.hpp>

#include "core/events.hpp"
#include "entt/entity/fwd.hpp"
#include "physics/components.hpp"
#include "physics/events.hpp"
#include "utility/utility.hpp"
#include "world/components.hpp"
#include "physics/system.hpp"

REGISTER_SYSTEM(PhysicsSystem);

void PhysicsSystem::init(entt::registry& reg) {
    subscribeGlobalEvent<UpdateEvent, &PhysicsSystem::update>(reg, this);
}

// Checks rect-tile overlap
bool checkTileCollision(const sf::FloatRect& worldRect, const TileMapComp& map) {
    const float squeeze = 31.f / 32.f;
    float tileSize = map.tileSize;

    int minTileX = (int)(std::floor(worldRect.position.x / tileSize));
    int maxTileX = (int)(std::floor((worldRect.position.x + worldRect.size.x * squeeze) / tileSize));
    int minTileY = (int)(std::floor(worldRect.position.y / tileSize));
    int maxTileY = (int)(std::floor((worldRect.position.y + worldRect.size.y * squeeze) / tileSize));

    for (int y = minTileY; y <= maxTileY; ++y) {
        for (int x = minTileX; x <= maxTileX; ++x) {
            // Out of bounds counts as wall
            if (x < 0 || x >= map.width || y < 0 || y >= map.height) {
                return true;
            }
            if (map.grid[x + y * map.width] == TileType::Wall) {
                return true;
            }
        }
    }

    return false;
}

sf::Vector2f resolveCollision(const sf::Vector2f& currentPos, const sf::Vector2f& desiredPos, const sf::FloatRect& bounds, const TileMapComp& map) {
    sf::Vector2f shift = desiredPos - currentPos;
    // Try moving only on X axis
    sf::Vector2f tryX = sf::Vector2f(desiredPos.x, currentPos.y);
    sf::FloatRect rectX = bounds;
    rectX.position += tryX;
    // Try moving only on Y axis
    sf::Vector2f tryY = sf::Vector2f(currentPos.x, desiredPos.y);
    sf::FloatRect rectY = bounds;
    rectY.position += tryY;
    // Try moving in full
    sf::FloatRect rectXY = bounds;
    rectXY.position += desiredPos;

    bool xBlocked = checkTileCollision(rectX, map);
    bool yBlocked = checkTileCollision(rectY, map);
    bool fullBlocked = checkTileCollision(rectXY, map);

    // Where to move to?
    sf::Vector2f endPos = currentPos;
    // Prevent getting stuck diagonally
    if (fullBlocked && !xBlocked && !yBlocked) {
        if (shift.x > shift.y) endPos.x = desiredPos.x;
        else endPos.y = desiredPos.y;
    } else {
        if (!xBlocked) endPos.x = desiredPos.x;
        if (!yBlocked) endPos.y = desiredPos.y;
    }
    return endPos;
}

void PhysicsSystem::update(const UpdateEvent& ev) {
    entt::registry& reg = *ev.registry;

    auto view = ev.registry->view<PositionComp, PhysicsComp>();
    for (auto [entity, pos, phys] : view.each()) {
        sf::Vector2f desiredPos = pos.position + phys.velocity * ev.dt;
        auto [world, desiredWorldPos] = Physics::getWorldPos(pos.parent, desiredPos, reg);

        // Check tile collision
        if (TileMapComp* mapComp = reg.try_get<TileMapComp>(world)) {
            if (phys.velocity != zeroVec) {
                // Check our post-move bounds
                sf::FloatRect desiredWorldRect = phys.bounds;
                desiredWorldRect.position += desiredWorldPos;

                // Will we be colliding?
                if (checkTileCollision(desiredWorldRect, *mapComp)) {
                    sf::Vector2f currentWorldPos = Physics::worldPos(entity, reg);
                    sf::Vector2f resolvedWorldPos = resolveCollision(currentWorldPos, desiredWorldPos, phys.bounds, *mapComp);
                    desiredPos = pos.position + resolvedWorldPos - currentWorldPos;

                    if (ev.dt != 0.f) {
                        phys.velocity = (desiredPos - pos.position) / ev.dt;
                    }
                }
            }
        }

        pos.position = desiredPos;

        // Update velocity, apply drag
        if (phys.velocity != zeroVec) {
            float drag = phys.drag;
            raiseLocalEvent(reg, entity, GetDragEvent(&drag));
            float dragBy = drag * ev.dt;
            if (phys.velocity.lengthSquared() <= dragBy * dragBy)
                phys.velocity = zeroVec;
            else
                phys.velocity -= phys.velocity.normalized() * dragBy;
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