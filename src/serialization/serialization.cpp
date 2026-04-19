#include <yaml-cpp/yaml.h>
#include <rfl/yaml.hpp>
#include <entt/entt.hpp>
#include "serialization/serialization.hpp"
#include "serialization/serialization_entt.hpp"

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
        YAML::Node node = serialize_entity(reg, entity);
        if (node["id"])
            entities.push_back(node);
    }

    root["entities"] = entities;
    return root;
}

void deserialize_entity(const YAML::Node& node, entt::registry& reg) {
    uint32_t entity_id = node["id"].as<uint32_t>();
    entt::entity entity = reg.create((entt::entity)(entity_id));
    deserialize_entity(node, reg, entity);
}

void deserialize_entity(const YAML::Node& node, entt::registry& reg, entt::entity into) {
    if (const YAML::Node& components = node["components"]) {
        for (const YAML::Node& comp : components) {
            std::string type_name = comp["id"].as<std::string>();
            ComponentSerializer::deserialize(type_name, comp["params"], into, &reg);
        }
    }
}

void deserialize_registry(const YAML::Node& root, entt::registry& reg) {
    reg.clear();

    entity_id_ser_map.clear();
    if (const YAML::Node& entities = root["entities"]) {
        for (const YAML::Node& entity : entities) {
            uint32_t entity_id = entity["id"].as<uint32_t>();
            entt::entity id_actual = reg.create(entt::entity(entity_id));
            entity_id_ser_map[entity_id] = id_actual;
        }
        for (const YAML::Node& entity : entities) {
            uint32_t entity_id = entity["id"].as<uint32_t>();
            deserialize_entity(entity, reg, entity_id_ser_map[entity_id]);
        }
    }
    entity_id_ser_map.clear();
}
