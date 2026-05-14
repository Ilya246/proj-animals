#pragma once
#include "core/events.hpp"
#include "core/system.hpp"
#include "input/events.hpp"

struct UISystem : System<UISystem> {
    entt::entity activeTextbox = entt::null;

    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return 32; };

        void update(const UpdateEvent&);
        void onScreenResize(const ScreenResizeEvent&);
        void onGlobalMouseMove(const GlobalMouseMoveEvent&);
        void onGlobalKeyPress(const KeyPressEvent&);
        void onGlobalTextEntered(const GlobalTextEnteredEvent&);
};

namespace UI {
    void rebuild(entt::entity node, entt::registry& reg);
}
