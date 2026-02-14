#pragma once
#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>
#include "core/events.hpp"
#include "core/system.hpp"

struct PhysicsSystem : SystemBase {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return 16; };

        void update(const UpdateEvent&);
};

namespace Physics {

entt::entity getWorld(const entt::entity&, const entt::registry&);
sf::Vector2f worldPos(const entt::entity&, const entt::registry&);
std::pair<entt::entity, sf::Vector2f> getWorldPos(const entt::entity&, const entt::registry&);
std::pair<entt::entity, sf::Vector2f> getWorldPos(const entt::entity& parent, const sf::Vector2f& pos, const entt::registry&);

}