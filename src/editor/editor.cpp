#include "editor/system.hpp"
#include "editor/components.hpp"
#include "physics/system.hpp"

entt::entity find_main_world(const entt::registry& reg) {
    auto view = reg.view<MainCameraComp>();

    for (auto [ent, mc] : view.each()) {
        return Physics::getWorld(ent, reg);
    }

    return (entt::entity)-1;
}
