#pragma once
#include <rfl.hpp>
#include <entt/entt.hpp>

struct EnttEntitySerializationHelper {
    uint32_t id;

    static EnttEntitySerializationHelper from_class(const entt::entity& e) noexcept {
        return EnttEntitySerializationHelper{static_cast<uint32_t>(e)};
    }

    entt::entity to_class() const noexcept{
        return static_cast<entt::entity>(id);
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, entt::entity, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType,
                          entt::entity, EnttEntitySerializationHelper> {};
}