#include <entt/entt.hpp>
#include "core/components.hpp"
#include "core/events.hpp"

entt::dispatcher& getGlobalDispatcher(entt::registry& reg) {
    return reg.ctx().get<entt::dispatcher>();
}

std::string get_ent_name(entt::entity& ent, entt::registry& reg) {
    if (auto* comp = reg.try_get<EntNameComp>(ent))
        return comp->name;
    return "unknown";
}

void set_ent_name(entt::entity& ent, entt::registry& reg, std::string_view to) {
    auto* comp = reg.try_get<EntNameComp>(ent);
    if (!comp)
        comp = &reg.emplace<EntNameComp>(ent);

    comp->name = to;
}
