#pragma once
#include "utility/utility.hpp"
#include <rfl.hpp>
#include <entt/entt.hpp>
#include <unordered_map>

// YAML entity -> registry entity map to prevent conflicts with existing entities
inline std::unordered_map<uint32_t, entt::entity> entity_id_ser_map = {};

struct EnttEntitySerializationHelper {
    uint32_t id;

    static EnttEntitySerializationHelper from_class(const entt::entity& e) noexcept {
        return EnttEntitySerializationHelper{static_cast<uint32_t>(e)};
    }

    entt::entity to_class() const noexcept {
        exec_debug([&]{ if (!entity_id_ser_map.contains(id)) std::cout << "[WARN]: requested serialization entity id mapping for " << id << ", but none found"; });
        return entity_id_ser_map.at(id);
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, entt::entity, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType,
                          entt::entity, EnttEntitySerializationHelper> {};
}