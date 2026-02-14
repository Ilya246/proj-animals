#pragma once
#include <memory>
#include <entt/entt.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

struct SystemBase {
    virtual ~SystemBase() = default;

    // Called in order on all systems after the registry and window exist.
    virtual void init(entt::registry&) {}

    // Override to initialise in an enforced order.
    virtual int initPriority() { return 0; };
};

inline std::vector<std::unique_ptr<SystemBase>> systems;

template<typename TSys>
struct SystemRegistrar {
    SystemRegistrar() {
        systems.push_back(std::make_unique<TSys>());
    }
};

#define REGISTER_SYSTEM(SystemType) \
    static SystemRegistrar<SystemType> __SystemType##_registrar
