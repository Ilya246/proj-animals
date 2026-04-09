#pragma once
#include <SFML/Graphics.hpp>
#include <rfl.hpp>
#include "graphics/events.hpp"
#include "graphics/texture.hpp"
#include "serialization/serialization.hpp"
#include "utility/base64.hpp"

enum class TileType : uint8_t {
    Ground = 0,
    Wall = 1
};

struct TileMapComp {
    int width;
    int height;
    float tileSize;

    // Texture atlas
    const sf::Texture* texture = nullptr;

    // TODO: replace with chunks?
    std::vector<TileType> grid{};

    // Vertices for batch rendering
    sf::VertexArray vertices{sf::PrimitiveType::Triangles};

    void OnRender(RenderEvent&);

    REGISTER_SERIALIZABLE(TileMapComp, TileMap)
};

namespace MapUtil {

void rebuildMesh(entt::entity ent, TileMapComp& map, entt::registry& reg);
sf::Vector2i getTilePos(const sf::Vector2f& pos, float tileSize);
TileType getTileAt(const sf::Vector2f& pos, const TileMapComp& map);

}

// serialization
struct TileMapCompHelper {
    int width;
    int height;
    float tileSize;
    std::string grid;
    std::string texture;
 
    static TileMapCompHelper from_class(const TileMapComp& map) noexcept {
        std::string_view tiles_view((char*)map.grid.data(), map.grid.size() * sizeof(TileType));
        return TileMapCompHelper{
            map.width,
            map.height,
            map.tileSize,
            base64_encode(tiles_view),
            tex_name_map[map.texture]
        };
    }
    
    TileMapComp to_class() const noexcept {
        TileMapComp map;
        map.width = width;
        map.height = height;
        map.tileSize = tileSize;
        std::string decoded = base64_decode(grid);
        map.grid = std::vector<TileType>((TileType*)decoded.data(), (TileType*)(decoded.data() + decoded.size()));
        map.texture = &tex_map[texture];
        return map;
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, TileMapComp, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          TileMapComp, TileMapCompHelper> {};
}

template<>
struct PostDeserialize<TileMapComp> {
    static void exec(entt::entity e, TileMapComp& comp, entt::registry* r) {
        MapUtil::rebuildMesh(e, comp, *r);
    }
};
