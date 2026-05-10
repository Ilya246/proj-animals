#pragma once
#include <entt/entt.hpp>
#include <SFML/Graphics/Rect.hpp>

struct UISizeAllocatedEvent {
    sf::FloatRect bounds;
};

struct UIPropagateEvent {
    std::function<void(entt::registry&, entt::entity)> action;
};

struct LayoutConstraints {
    float minWidth = 0.f;
    float minHeight = 0.f;
    bool expandX = false;
    bool expandY = false;
    float weightX = 1.f;
    float weightY = 1.f;
};

struct UIQueryChildEvent {
    LayoutConstraints& constraints;
};
