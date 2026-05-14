#pragma once
#include "graphics/events.hpp"
#include "serialization/serialization.hpp"
#include "core/events.hpp"

// Component to designate the camera considered primary. Used for editor.
struct MainCameraComp {
    bool _dummy; // Required for EnTT

    REGISTER_SERIALIZABLE(MainCameraComp, MainCamera)
};

struct EditorSelectedComp {
    bool _dummy; // Required for EnTT

    HANDLE_EVENT(EditorSelectedComp, RenderEvent)
};

struct EditorComp {
    bool _dummy;
};

struct ComponentEditorUIComp {
    entt::entity targetEntity = entt::null;
    std::string selectedComponent = "";
    std::string lastSelectedComponent = "";

    entt::entity paramListContainer = entt::null;
    std::unordered_map<std::string, entt::entity> paramBoxes = {};
};
