#include <rfl.hpp>
#include <entt/entt.hpp>

// ============================================================================
// Custom Parser for entt::entity (CRITICAL: overrides enum handling)
// Since entt::entity is an enum class, reflect-cpp's enum parser would
// normally kick in. We need to explicitly override it to treat it as uint32_t.
// ============================================================================

// Helper struct for entt::entity serialization
struct EnttEntityHelper {
    uint32_t id;
    
    static EnttEntityHelper from_class(const entt::entity& e) noexcept {
        return EnttEntityHelper{static_cast<uint32_t>(e)};
    }
    
    entt::entity to_class() const noexcept {
        return static_cast<entt::entity>(id);
    }
};

// Parser specialization that overrides the enum parser for entt::entity
namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, entt::entity, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          entt::entity, EnttEntityHelper> {};
}