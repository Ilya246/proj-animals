#pragma once
#include "entt/entity/fwd.hpp"
#include <SFML/Graphics.hpp>

struct RenderEvent {
    entt::entity ent;
    entt::registry* reg;
    sf::RenderWindow* window;
};