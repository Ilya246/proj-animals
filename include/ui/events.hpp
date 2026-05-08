#pragma once
#include <entt/entt.hpp>
#include <SFML/Graphics/Rect.hpp>

struct UISizeAllocatedEvent {
    sf::FloatRect bounds;
};

struct UIPropagateEvent {
    std::function<void(entt::registry&, entt::entity)> action;
};
