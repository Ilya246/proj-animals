#pragma once
#include "core/events.hpp"
#include "core/system.hpp"
#include <entt/entt.hpp>

void queue_delete(entt::entity ent, entt::registry& reg);

struct CoreSystem : System<CoreSystem> {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return -99999; };

        void onUpdate(UpdateEvent& ev);
        void queueDelete(entt::entity ent);

        std::vector<entt::entity> delete_queue;

        friend void queue_delete(entt::entity ent, entt::registry& reg);
};

// Fired on an entity just before it is deleted.
struct EntityDeleteEvent {
    bool _dummy;
};
