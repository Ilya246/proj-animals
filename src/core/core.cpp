#include <entt/entity/fwd.hpp>
#include "core/events.hpp"

entt::dispatcher& getGlobalDispatcher(entt::registry& reg) {
    return reg.ctx().get<entt::dispatcher>();
}
