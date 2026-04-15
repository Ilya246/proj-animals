#pragma once
#include <entt/entt.hpp>
#include <SFML/Graphics/Rect.hpp>

struct UISizeAllocatedEvent {
    sf::FloatRect bounds;
    entt::registry* registry;
    entt::entity entity;
};
