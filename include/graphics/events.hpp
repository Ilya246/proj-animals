#pragma once
#include "entt/entity/fwd.hpp"
#include <SFML/Graphics.hpp>

struct RenderEvent {
    entt::entity ent;
    entt::registry* reg;
    sf::RenderWindow* window;
};

struct ShouldRenderEvent {
    entt::entity ent;
    entt::registry* reg;
    bool* cancelled;
    // stencil bounds, in world coordinates
    std::optional<sf::FloatRect>* stencil;
};
