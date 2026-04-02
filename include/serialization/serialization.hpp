#pragma once
#include <rfl/yaml.hpp>
#include <yaml-cpp/yaml.h>
#include <entt/entt.hpp>
#include <unordered_map>
#include <functional>
#include <string>
#include <cassert>

#include "serialization_entt.hpp"
#include "serialization_sfml.hpp"

template<typename T>
struct PostDeserialize {
    // static void exec(entt::entity e, T& comp, entt::registry* r);
};

template<typename T>
concept HasPostDeserializeExec = requires(entt::entity e, T& comp, entt::registry* r) {
    PostDeserialize<T>::exec(e, comp, r);
};

struct ComponentSerializer {
    template<typename T>
    static void register_component(const std::string& name) {
        Registry& registry = get_registry();

        registry.deserialize[name] = [](const YAML::Node& node, entt::entity e, entt::registry* r) {
            auto result = rfl::yaml::read<T>(node);
            if (result) {
                r->emplace<T>(e, std::move(*result));
                if constexpr (HasPostDeserializeExec<T>) {
                    PostDeserialize<T>::exec(e, r->get<T>(e), r);
                }
            }
        };

        registry.serialize[name] = [](entt::registry& r, entt::entity e) -> std::optional<YAML::Node> {
            if (T* comp = r.try_get<T>(e)) {
                return YAML::Load(rfl::yaml::write(*comp));
            }
            return {};
        };

        registry.type_names.push_back(name);
    }

    static void deserialize(const std::string& name, const YAML::Node& yaml_str,
                           entt::entity e, entt::registry* r) {
        Registry& registry = get_registry();
        if (auto it = registry.deserialize.find(name); it != registry.deserialize.end()) {
            it->second(yaml_str, e, r);
        }
    }

    static std::optional<YAML::Node> serialize(const std::string& name, entt::registry& r, entt::entity e) {
        Registry& registry = get_registry();
        if (auto it = registry.serialize.find(name); it != registry.serialize.end()) {
            return it->second(r, e);
        }
        return {};
    }

    static const std::vector<std::string>& get_registered_types() {
        return get_registry().type_names;
    }

    struct Registry {
        std::unordered_map<std::string, std::function<void(const YAML::Node&, entt::entity, entt::registry*)>> deserialize;
        std::unordered_map<std::string, std::function<std::optional<YAML::Node>(entt::registry&, entt::entity)>> serialize;
        std::vector<std::string> type_names;
    };

    static Registry& get_registry() {
        static Registry registry;
        return registry;
    }
};

YAML::Node serialize_entity(entt::registry& reg, entt::entity e);

YAML::Node serialize_registry(entt::registry& reg);

void deserialize_entity(const YAML::Node& node, entt::registry& reg);

void deserialize_registry(const YAML::Node& root, entt::registry& reg);
