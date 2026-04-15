#pragma once
#include "core/events.hpp"
#include "core/system.hpp"

struct UISystem : System<UISystem> {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return -16; };
        void onScreenResize(const ScreenResizeEvent&);
};
