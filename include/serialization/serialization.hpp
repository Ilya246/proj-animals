#pragma once
#include <rfl/yaml.hpp>
#include <yaml-cpp/yaml.h>
#include <entt/entt.hpp>
#include <unordered_map>
#include <functional>
#include <string>
#include <iostream>
#include <cassert>

#include "serialization_entt.hpp"
#include "serialization_sfml.hpp"

// ============================================================================
// Component Serialization System
// ============================================================================

class ComponentSerializer {
public:
    using SerializeFunc = std::function<std::string(entt::registry&, entt::entity)>;
    using DeserializeFunc = std::function<void(const std::string&, entt::entity, entt::registry*)>;
    
    template<typename T>
    static bool register_component(const std::string& name) {
        auto& registry = get_registry();
        
        registry.deserialize[name] = [](const std::string& yaml_str, entt::entity e, entt::registry* r) {
            auto result = rfl::yaml::read<T>(yaml_str);
            if (result) {
                r->emplace<T>(e, std::move(*result));
            }
        };
        
        registry.serialize[name] = [](entt::registry& r, entt::entity e) -> std::string {
            if (auto* comp = r.try_get<T>(e)) {
                return rfl::yaml::write(*comp);
            }
            return "";
        };
        
        registry.type_names.push_back(name);
        std::cout << "Registered component type: " << name << std::endl;
        return true;
    }
    
    static void deserialize(const std::string& name, const std::string& yaml_str, 
                           entt::entity e, entt::registry* r) {
        auto& registry = get_registry();
        if (auto it = registry.deserialize.find(name); it != registry.deserialize.end()) {
            it->second(yaml_str, e, r);
        }
    }
    
    static std::string serialize(const std::string& name, entt::registry& r, entt::entity e) {
        auto& registry = get_registry();
        if (auto it = registry.serialize.find(name); it != registry.serialize.end()) {
            return it->second(r, e);
        }
        return "";
    }
    
    static const std::vector<std::string>& get_registered_types() {
        return get_registry().type_names;
    }
    
private:
    struct Registry {
        std::unordered_map<std::string, DeserializeFunc> deserialize;
        std::unordered_map<std::string, SerializeFunc> serialize;
        std::vector<std::string> type_names;
    };
    
    static Registry& get_registry() {
        static Registry registry;
        return registry;
    }
};

// ============================================================================
// Entity Serialization Functions
// ============================================================================

YAML::Node serialize_entity(entt::registry& reg, entt::entity e);

YAML::Node serialize_registry(entt::registry& reg);

void deserialize_entity(const YAML::Node& node, entt::registry& reg);

void deserialize_registry(const YAML::Node& root, entt::registry& reg);