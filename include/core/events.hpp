#pragma once
#include <entt/entt.hpp>
#include "core/components.hpp"

// Published every update
struct UpdateEvent {
    float dt;
    entt::registry* registry;
};

// Helper method to get the event dispatcher
entt::dispatcher& getGlobalDispatcher(entt::registry&);

inline entt::dispatcher& getEntDispatcher(entt::registry& reg, entt::entity& ent) {
    EventDispatchComp* comp = nullptr;
    if (!(comp = reg.try_get<EventDispatchComp>(ent)))
        comp = &reg.emplace<EventDispatchComp>(ent);
    return comp->dispatcher;
}

template<typename TComp, typename TEv, auto Fun>
inline void __ensureSubscribe(entt::registry& reg, entt::entity ent) {
    getEntDispatcher(reg, ent).sink<TEv>().template connect<Fun>(reg.get<TComp>(ent));
}

template<typename TComp, typename TEv, auto Fun>
inline void __ensureUnsubscribe(entt::registry& reg, entt::entity ent) {
    getEntDispatcher(reg, ent).sink<TEv>().template disconnect<Fun>(reg.get<TComp>(ent));
}

// Subscribe specified component to an event.
// When raise_local_event is raised for that event on our entity, run the function.
template<typename TComp, typename TEv, auto Fun>
inline void subscribe_local_event(entt::registry& reg) {
    reg.on_construct<TComp>().template connect<&__ensureSubscribe<TComp, TEv, Fun>>();
    reg.on_destroy<TComp>().template connect<&__ensureUnsubscribe<TComp, TEv, Fun>>();
}

// Dispatch an event to an entity's components.
// Components subscribing to the event will run code.
template<typename TEv>
inline void raise_local_event(entt::registry& reg, entt::entity& ent, TEv&& ev) {
    if (EventDispatchComp* dispatch = reg.try_get<EventDispatchComp>(ent))
        dispatch->dispatcher.trigger(ev);
}

// Make some function happen when an event is triggered.
template<typename TEv, auto Fun>
inline void subscribe_global_event(entt::registry& reg) {
    getGlobalDispatcher(reg).sink<TEv>().template connect<Fun>();
}

// Make some function on an object happen when an event is triggered.
template<typename TEv, auto Fun, typename TSub>
inline void subscribe_global_event(entt::registry& reg, TSub* instance) {
    getGlobalDispatcher(reg).sink<TEv>().template connect<Fun>(instance);
}