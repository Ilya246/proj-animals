#pragma once
#include "core/events.hpp"
#include "core/system.hpp"

struct DrawSystem : System<DrawSystem> {
    private:
        virtual void init(entt::registry&) override;

        void update(const UpdateEvent&);
};
