#pragma once
#include "serialization/serialization.hpp"
#include <entt/entt.hpp>

struct EntNameComp {
    std::string name = "";

    REGISTER_SERIALIZABLE(EntNameComp, EntName)
};

std::string get_ent_name(entt::entity& ent, entt::registry& reg);
void set_ent_name(entt::entity& ent, entt::registry& reg, std::string_view to);
