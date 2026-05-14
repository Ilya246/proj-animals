#include <entt/entt.hpp>
#include "core/components.hpp"
#include "core/events.hpp"
#include "core/entity.hpp"
#include "entt/entity/fwd.hpp"

void CoreSystem::init(entt::registry& reg) {
    subscribe_global_event<UpdateEvent, &CoreSystem::onUpdate>(reg, this);
}

void CoreSystem::onUpdate(UpdateEvent& ev) {
    for (entt::entity e : delete_queue) {
        if (ev.registry->valid(e))
            ev.registry->destroy(e);
    }
    delete_queue.clear();
}

void CoreSystem::queueDelete(entt::entity ent) {
    delete_queue.push_back(ent);
}

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

void queue_delete(entt::entity ent, entt::registry& reg) {
    CoreSystem& core = reg.ctx().get<CoreSystem>();
    raise_local_event(reg, ent, EntityDeleteEvent{});
    core.queueDelete(ent);
}
