#pragma once
#include <memory>
#include <entt/entt.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <queue>

struct SystemBase {
    virtual ~SystemBase() = default;

    // Called in order on all systems after the registry and window exist.
    virtual void init(entt::registry&) {}

    // Override to initialise in an enforced order.
    virtual int initPriority() { return 0; };
};

inline std::vector<std::unique_ptr<SystemBase>>& get_systems() {
    static std::vector<std::unique_ptr<SystemBase>> systems;
    return systems;
}

inline void init_systems(entt::registry& registry) {
    // Initialise systems
    std::priority_queue<std::pair<int, SystemBase*>> init_queue;
    for (auto& sys_ptr : get_systems()) {
        init_queue.push({sys_ptr->initPriority(), sys_ptr.get()});
    }
    while (init_queue.size() != 0) {
        auto& sys = init_queue.top().second;
        sys->init(registry);
        init_queue.pop();
    }
}

template<typename TSys>
struct System : SystemBase {
    struct Registrar {
        Registrar() {
            std::unique_ptr<TSys> ptr = std::make_unique<TSys>();
            system = ptr.get();
            get_systems().push_back(std::move(ptr));
        }
    };

    inline static Registrar registrar = {};
    inline static TSys* system;

    // hack so registrar is instantiated
    static_assert(&registrar != (void*)0);
};