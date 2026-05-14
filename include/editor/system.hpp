#pragma once
#include <entt/entt.hpp>
#include "core/events.hpp"
#include "core/system.hpp"
#include "input/events.hpp"

enum class EditorMode {
    None,
    Select,
    Spawn
};

struct EditorSystem : System<EditorSystem> {
    EditorMode mode = EditorMode::None;

private:
    virtual void init(entt::registry& reg) override;
    virtual int initPriority() override { return -256; }

    void onGlobalClick(const GlobalClickEvent& ev);
    void update(const UpdateEvent& ev);
};

entt::entity find_main_world(const entt::registry& reg);
