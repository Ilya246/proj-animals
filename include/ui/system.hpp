#pragma once
#include "core/system.hpp"

struct UISystem : System<UISystem> {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return 0; };
};