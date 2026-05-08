#pragma once
#include <entt/entt.hpp>
#include <type_traits>

// Published every update
struct UpdateEvent {
    float dt;
    entt::registry* registry;
};

struct ScreenResizeEvent {
    unsigned int width;
    unsigned int height;
    entt::registry* registry;
};

// Helper method to get the event dispatcher
entt::dispatcher& getGlobalDispatcher(entt::registry&);

template<typename TComp, typename TEv>
void handle_event(TEv& ev, entt::entity entity, TComp& comp, entt::registry& reg);

template<typename TEv>
inline std::vector<std::function<void(TEv&, entt::entity, entt::registry&)>> evSubs;

template<typename TComp, typename TEv>
inline void try_handle_event(TEv& ev, entt::entity ent, entt::registry& reg) {
    if (auto* comp = reg.try_get<TComp>(ent)) {
        handle_event(ev, ent, *comp, reg);
    }
}

template<typename TComp, typename TEv>
struct event_sub {
    event_sub() {
        evSubs<TEv>.push_back(try_handle_event<TComp, TEv>);
    }
    constexpr bool _dummy() const {
        return true;
    }
};

#define HANDLE_EVENT(Type, Event) \
    static inline const event_sub<Type, Event> _##Event##_handler; \
    static_assert(_##Event##_handler._dummy());

// Dispatch an event to an entity's components.
// Components subscribing to the event will run code.
template<typename TEv>
inline void raise_local_event(entt::registry& reg, entt::entity& ent, TEv&& ev) {
    using BaseEv = std::remove_cvref_t<TEv>;
    for (auto& sub : evSubs<BaseEv>) {
        sub(ev, ent, reg);
    }
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
