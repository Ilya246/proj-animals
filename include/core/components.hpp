#pragma once
#include <entt/entt.hpp>

// Allows dispatching an event to components of an entity that want to subscribe to it.
struct EventDispatchComp {
    entt::dispatcher dispatcher{};
};