#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entt.hpp>

#include "core/events.hpp"
#include "entt/entity/fwd.hpp"
#include "physics/components.hpp"
#include "physics/events.hpp"
#include "utility/utility.hpp"
#include "physics/system.hpp"

void PhysicsSystem::init(entt::registry& reg) {
    subscribe_global_event<UpdateEvent, &PhysicsSystem::update>(reg, this);
}

void for_chunk(int enlarge, entt::registry& reg, entt::entity ent, PositionComp& pos, ColliderMapComp& map, std::function<void(int32_t, int32_t)>&& fun) {
    sf::FloatRect bounds = reg.get<BoundsComp>(ent).bounds;
    bounds.position += pos.position;
    sf::Vector2f topRight = bounds.position + bounds.size;
    auto bLChunk = map.to_chunk(bounds.position);
    auto tRChunk = map.to_chunk(topRight);
    for (int32_t x = bLChunk.first - enlarge; x <= tRChunk.first + enlarge; ++x) {
        for (int32_t y = bLChunk.second - enlarge; y <= tRChunk.second + enlarge; ++y) {
            fun(x, y);
        }
    }
}

// returns: {P_i.x, P_i.y, t_A, t_B}
sf::FloatRect line_line_intersect(sf::Vector2f A1, sf::Vector2f A2, sf::Vector2f B1, sf::Vector2f B2) {
    sf::Vector2f A = A2 - A1;
    sf::Vector2f B = B2 - B1;
    float f_A = A1.x * A2.y - A1.y * A2.x;
    float f_B = B1.x * B2.y - B1.y * B2.x;
    float denom = A.x * B.y - A.y * B.x;
    sf::Vector2f P_i = {(-f_A * B.x + A.x * f_B) / denom, (-f_A * B.y + A.y * f_B) / denom};
    float t_A = (A.dot(P_i) - A.dot(A1)) / A.lengthSquared();
    float t_B = (B.dot(P_i) - B.dot(B1)) / B.lengthSquared();
    return {P_i, {t_A, t_B}};
}

void PhysicsSystem::update(const UpdateEvent& ev) {
    entt::registry& reg = *ev.registry;

    // Populate spatial hashes
    auto colliderMapView = ev.registry->view<ColliderMapComp>();
    for (auto [entity, map] : colliderMapView->each()) {
        map.spatial_map.clear();
    }

    auto colliderView = ev.registry->view<PositionComp, ColliderComp>();
    for (auto [entity, pos, collider] : colliderView.each()) {
        ColliderMapComp* map = reg.try_get<ColliderMapComp>(pos.parent);
        if (!map)
            map = &reg.emplace<ColliderMapComp>(pos.parent);

        for_chunk(0, reg, entity, pos, *map, [&](int32_t x, int32_t y) { map->spatial_map[map->chunk_hash(x, y)].push_back(entity); });
    }

    auto view = ev.registry->view<PhysicsComp, PositionComp>();
    for (auto [entity, phys, pos] : view.each()) {
        // Update velocity, apply drag
        if (phys.velocity != zeroVec) {
            // Process collisions
            sf::Vector2f desiredMove = phys.velocity * ev.dt;
            bool move_blocked = false;

            // Check collision
            if (ColliderComp* collider = reg.try_get<ColliderComp>(entity)) {
                move_blocked = collider->move_blocked;
                collider->move_blocked = false;
                ColliderMapComp& map = reg.get<ColliderMapComp>(pos.parent);

                std::unordered_set<entt::entity> intersecting;
                for_chunk(1, reg, entity, pos, map, [&](int32_t x, int32_t y) {
                    std::vector<entt::entity> chunkEnts = map.spatial_map[map.chunk_hash(x, y)];
                    for (entt::entity cEnt : chunkEnts)
                        intersecting.insert(cEnt);
                });
                sf::FloatRect ourBounds = reg.get<BoundsComp>(entity).bounds;
                for (entt::entity iEnt : intersecting) {
                    if (iEnt == entity)
                        continue;

                    sf::Vector2f otherVel = zeroVec;
                    PhysicsComp* otherPhys = reg.try_get<PhysicsComp>(iEnt);
                    if (otherPhys)
                        otherVel = otherPhys->velocity;
                    sf::Vector2f relVel = phys.velocity - otherVel;
                    sf::Vector2f relMove = relVel * ev.dt;

                    PositionComp& otherPosComp = reg.get<PositionComp>(iEnt);
                    sf::Vector2f otherPos = otherPosComp.position;
                    sf::FloatRect otherBounds = reg.get<BoundsComp>(iEnt).bounds;
                    // reduce this to a point-rect intersect with us at 0, 0, rect being A1A2A3A4
                    sf::FloatRect relBounds = sf::FloatRect{otherBounds.position + otherPos - ourBounds.position - pos.position - ourBounds.size, otherBounds.size + ourBounds.size};

                    sf::Vector2f A1 = relBounds.position;
                    sf::Vector2f A2 = {relBounds.position.x, relBounds.position.y + relBounds.size.y};
                    sf::Vector2f A3 = relBounds.position + relBounds.size;
                    sf::Vector2f A4 = {relBounds.position.x + relBounds.size.x, relBounds.position.y};

                    sf::FloatRect intersects[4];
                    intersects[0] = line_line_intersect(zeroVec, relMove, A1, A2);
                    intersects[1] = line_line_intersect(zeroVec, relMove, A2, A3);
                    intersects[2] = line_line_intersect(zeroVec, relMove, A3, A4);
                    intersects[3] = line_line_intersect(zeroVec, relMove, A4, A1);

                    sf::FloatRect best {{0.f, 0.f}, {2.f, 1.f}};
                    int best_i;
                    for (int i = 0; i < 4; ++i) {
                        sf::FloatRect& candidate = intersects[i];
                        if (candidate.size.x >= 0.f && candidate.size.y >= 0.f && candidate.size.y <= 1.f && candidate.size.x < best.size.x) {
                            best = candidate;
                            best_i = i;
                        }
                    }
                    if (best.size.x < 1.f) {
                        // we collided, go ahead and process symmetrically
                        sf::Vector2f normal;
                        switch (best_i) {
                            case (0): {
                                normal = {-1.f, 0.f};
                                break;
                            }
                            case (1): {
                                normal = {0.f, 1.f};
                                break;
                            }
                            case (2): {
                                normal = {1.f, 0.f};
                                break;
                            }
                            case (3): {
                                normal = {0.f, -1.f};
                                break;
                            }
                        }
                        // see if we're actually colliding
                        float vRelDotN = relVel.dot(normal);
                        if (vRelDotN < 0.f) {
                            desiredMove *= best.size.x;
                            ColliderComp& otherCollider = reg.get<ColliderComp>(iEnt);
                            move_blocked = true;
                            otherPosComp.position += otherVel * ev.dt * best.size.x;
                            pos.position += phys.velocity * ev.dt * best.size.x;
                            float restitution = collider->restitution * otherCollider.restitution;
                            float m1 = collider->mass;
                            float m2 = otherCollider.mass;
                            float massSum = m1 + m2;
                            float frac1 = m1 / massSum;
                            float frac2 = m2 / massSum;
                            if (!otherPhys) {
                                frac1 = 1.f;
                                frac2 = 0.f;
                            }
                            float d_rvel = (1.f + restitution) * -vRelDotN;
                            phys.velocity += normal * d_rvel * frac1;
                            if (otherPhys)
                                otherPhys->velocity -= normal * d_rvel * frac2;
                        }
                    }
                }
            }

            // if our move is blocked from a collision, don't move but still do other things
            if (!move_blocked)
                pos.position += desiredMove;

            float drag = phys.drag;
            raise_local_event(reg, entity, GetDragEvent(&drag));
            float dragBy = drag * ev.dt;
            if (phys.velocity.lengthSquared() <= dragBy * dragBy)
                phys.velocity = zeroVec;
            else
                phys.velocity -= phys.velocity.normalized() * dragBy;
        }
    }
}

void BoundsComp::resize(const sf::FloatRect& newBounds, entt::entity e, entt::registry& reg) {
    bounds = newBounds;
    raise_local_event(reg, e, BoundsResizeEvent{&reg, e, newBounds});
}

std::pair<int32_t, int32_t> ColliderMapComp::to_chunk(sf::Vector2f pos) {
    int32_t x_comp = pos.x / chunk_size;
    int32_t y_comp = pos.y / chunk_size;
    return {x_comp, y_comp};
}

uint64_t ColliderMapComp::to_hash(sf::Vector2f pos) {
    auto chunk = to_chunk(pos);
    return chunk_hash(chunk.first, chunk.second);
}

uint64_t ColliderMapComp::chunk_hash(int32_t x, int32_t y) {
    return (int64_t)x | ((int64_t)y << 32);
}

namespace Physics {

entt::entity getWorld(const entt::entity& ent, const entt::registry& reg) {
    const PositionComp* comp = &reg.get<PositionComp>(ent);
    const entt::entity* cEnt = &ent;
    while (comp->parent != *cEnt) {
        cEnt = &comp->parent;
        comp = &reg.get<PositionComp>(*cEnt);
    }
    return *cEnt;
}

std::pair<entt::entity, sf::Vector2f> getWorldAndPos(const entt::entity& ent, const entt::registry& reg) {
    const PositionComp* comp = &reg.get<PositionComp>(ent);
    const entt::entity* cEnt = &ent;
    sf::Vector2f pos = comp->position;
    while (comp->parent != *cEnt) {
        cEnt = &comp->parent;
        comp = &reg.get<PositionComp>(*cEnt);
        pos += comp->position;
    }
    return {*cEnt, pos};
}

sf::Vector2f getWorldPos(const entt::entity& ent, const entt::registry& reg) {
    return getWorldAndPos(ent, reg).second;
}

std::pair<entt::entity, sf::Vector2f> getWorldAndPos(const entt::entity& parent, const sf::Vector2f& pos, const entt::registry& reg) {
    auto lower = getWorldAndPos(parent, reg);
    return {lower.first, lower.second + pos};
}

}