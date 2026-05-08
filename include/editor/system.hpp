#pragma once
#include <entt/entt.hpp>
#include "core/system.hpp"
#include "input/events.hpp"

enum class EditorMode {
    Select,
    Delete,
    Spawn,
    AddComp
};

struct EditorSystem : System<EditorSystem> {
    bool editorActive = false;
    EditorMode mode = EditorMode::Select;

private:
    virtual void init(entt::registry& reg) override;
    virtual int initPriority() override { return 70; }

    void onGlobalClick(const GlobalClickEvent& ev);
    void onKeyPress(const KeyPressEvent& ev);
};

entt::entity find_main_world(const entt::registry& reg);
