#pragma once

#include <entt/entt.hpp>

// Published every update
struct UpdateEvent {
    float dt;
    entt::registry* registry;
};

// Helper method to get the event dispatcher
entt::dispatcher& ev_dispatcher(entt::registry&);