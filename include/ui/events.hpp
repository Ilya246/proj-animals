#pragma once
#include <entt/entt.hpp>
#include <SFML/Graphics/Rect.hpp>

struct UISizeAllocatedEvent {
    sf::FloatRect bounds;
    entt::registry* registry;
    entt::entity entity;
};

struct UIPropagateEvent {
    entt::registry* registry;
    entt::entity entity;
    std::function<void(entt::registry&, entt::entity)> action;
};
