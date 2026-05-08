#pragma once
#include <functional>
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

inline std::vector<std::function<std::unique_ptr<SystemBase>(entt::registry&)>>& get_system_creators() {
    static std::vector<std::function<std::unique_ptr<SystemBase>(entt::registry&)>> systems;
    return systems;
}

inline std::vector<std::unique_ptr<SystemBase>> create_systems(entt::registry& registry) {
    // Initialise systems
    std::priority_queue<std::pair<int, std::unique_ptr<SystemBase>>> init_queue;
    for (auto& sys_creator : get_system_creators()) {
        std::unique_ptr<SystemBase> sys = sys_creator(registry);
        init_queue.push({sys->initPriority(), std::move(sys)});
    }
    std::vector<std::unique_ptr<SystemBase>> systems;
    while (init_queue.size() != 0) {
        auto& sys = const_cast<std::unique_ptr<SystemBase>&>(init_queue.top().second);
        sys->init(registry);
        systems.push_back(std::move(sys));
        init_queue.pop();
    }

    return systems;
}

template<typename TSys>
struct System : SystemBase {
    struct Registrar {
        Registrar() {
            auto fun = [] (entt::registry& reg) {
                std::unique_ptr<TSys> ptr = std::make_unique<TSys>();
                reg.ctx().emplace<TSys&>(*ptr.get());
                return ptr;
            };
            get_system_creators().push_back(fun);
        }
    };

    inline static Registrar registrar = {};
    inline static TSys* system;

    // hack so registrar is instantiated
    static_assert(&registrar != (void*)0);
};
