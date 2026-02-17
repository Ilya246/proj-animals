#pragma once
#include "core/events.hpp"
#include "core/system.hpp"

struct InputSystem : System<InputSystem> {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return 64; };

        void update(const UpdateEvent&);
};
