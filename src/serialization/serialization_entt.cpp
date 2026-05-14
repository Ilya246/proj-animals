#include "core/components.hpp"
#include "serialization/serialization.hpp"

template<>
struct PostDeserialize<entt::entity> {
    static void exec(entt::entity e, entt::entity& id, entt::registry* r) {
        if (!r->valid(id)) {
            std::cout << std::format("[WARN] Deserialized invalid entity ID {} on entity {}.", (int)id, get_ent_name(e, *r)) << std::endl;
            id = entt::null;
        }
    }
};
