#include "core/system.hpp"

struct WorldSystem : System<WorldSystem> {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return 96; };
};
