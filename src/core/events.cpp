#include "core/events.hpp"

entt::dispatcher& ev_dispatcher(entt::registry& reg) {
    return reg.ctx().get<entt::dispatcher>();
}