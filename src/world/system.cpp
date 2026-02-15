#include "world/components.hpp"

namespace MapUtil {

void rebuildMesh(TileMapComp& map) {
    // 2 triangles per tile * 3 vertices per triangle
    map.vertices.resize(static_cast<std::size_t>(map.width * map.height * 6));

    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            // Get tile index
            int idx = x + y * map.width;
            TileType type = map.grid[idx];

            // Calculate vertex array index (6 verts per tile)
            std::size_t vIdx = idx * 6;

            // Pointer to the current quad's vertices
            sf::Vertex* quad = &map.vertices[vIdx];

            // Position calculation
            float ts = map.tileSize;
            float px = x * ts;
            float py = -y * ts - ts; // invert: convert from world-cord into draw-coord

            // Define the 6 vertices for 2 triangles (Quad replacement)
            // Triangle 1 (Top-Left, Top-Right, Bottom-Left)
            quad[0].position = {px, py};
            quad[1].position = {px + ts, py};
            quad[2].position = {px, py + ts};

            // Triangle 2 (Bottom-Left, Top-Right, Bottom-Right)
            quad[3].position = {px, py + ts};
            quad[4].position = {px + ts, py};
            quad[5].position = {px + ts, py + ts};

            // UV MAPPING (Texture Coordinates)
            // Assuming texture is a 2x1 atlas: Left=Ground, Right=Wall
            // You should calculate this based on texture size and tile index
            float u = (type == TileType::Wall) ? ts : 0.f;
            float v = 0.f;

            quad[0].texCoords = {u, v};
            quad[1].texCoords = {u + ts, v};
            quad[2].texCoords = {u, v + ts};

            quad[3].texCoords = {u, v + ts};
            quad[4].texCoords = {u + ts, v};
            quad[5].texCoords = {u + ts, v + ts};

            // Optional: Lighting/Color
            sf::Color color = sf::Color::White;
            if (type == TileType::Wall) color = {200, 200, 200}; // Slightly darker

            for (int i = 0; i < 6; ++i) quad[i].color = color;
        }
    }
}


sf::Vector2i getTilePos(const sf::Vector2f& pos, float tileSize) {
    int x = (int)(pos.x / tileSize);
    int y = (int)(pos.y / tileSize);
    return {x, y};
}

TileType getTileAt(const sf::Vector2f& pos, const TileMapComp& map) {
    sf::Vector2i tilePos = getTilePos(pos, map.tileSize);

    if (tilePos.x < 0 || tilePos.x >= map.width || tilePos.y < 0 || tilePos.y >= map.height) 
        return TileType::Wall; // Bounds is wall

    return map.grid[tilePos.x + tilePos.y * map.width];
}

}
