#pragma once
#include <SFML/Graphics.hpp>

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
};

namespace MapUtil {

void rebuildMesh(TileMapComp& map);
sf::Vector2i getTilePos(const sf::Vector2f& pos, float tileSize);
TileType getTileAt(const sf::Vector2f& pos, const TileMapComp& map);

}