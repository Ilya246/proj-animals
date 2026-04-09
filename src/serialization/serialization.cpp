#include <yaml-cpp/yaml.h>
#include <rfl/yaml.hpp>
#include <entt/entt.hpp>
#include "serialization/serialization.hpp"

YAML::Node serialize_entity(entt::registry& reg, entt::entity e) {
    if (auto _ = reg.try_get<NonSerializableComp>(e))
        return {};

    YAML::Node node;
    node["id"] = static_cast<uint32_t>(e);

    YAML::Node components;
    for (const std::string& type_name : ComponentSerializer::get_registered_types()) {
        std::optional<YAML::Node> node = ComponentSerializer::serialize(type_name, reg, e);
        if (node) {
            YAML::Node comp;
            comp["id"] = type_name;
            comp["params"] = *node;
            components.push_back(comp);
        }
    }

    node["components"] = components;
    return node;
}

YAML::Node serialize_registry(entt::registry& reg) {
    YAML::Node root;
    YAML::Node entities;

    for (auto [entity] : reg.view<entt::entity>().each()) {
        entities.push_back(serialize_entity(reg, entity));
    }

    root["entities"] = entities;
    return root;
}

void deserialize_entity(const YAML::Node& node, entt::registry& reg) {
    uint32_t entity_id = node["id"].as<uint32_t>();
    entt::entity entity = reg.create(static_cast<entt::entity>(entity_id));

    if (const YAML::Node& components = node["components"]) {
        for (const YAML::Node& comp : components) {
            std::string type_name = comp["id"].as<std::string>();
            ComponentSerializer::deserialize(type_name, comp["params"], entity, &reg);
        }
    }
}

void deserialize_registry(const YAML::Node& root, entt::registry& reg) {
    reg.clear();

    if (const YAML::Node& entities = root["entities"]) {
        for (const YAML::Node& entity : entities) {
            deserialize_entity(entity, reg);
        }
    }
}
