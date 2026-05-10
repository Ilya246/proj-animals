#pragma once
#include <entt/entt.hpp>
#include <SFML/Graphics/Rect.hpp>

struct UISizeAllocatedEvent {
    sf::FloatRect bounds;
};

struct UIPropagateEvent {
    std::function<void(entt::registry&, entt::entity)> action;
};

struct UIQueryDesiredSizeEvent {
    sf::Vector2f desiredSize{0.f, 0.f};
    float availableWidth = -1.f; // -1.f means infinite space
};
