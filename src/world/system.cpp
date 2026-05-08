#include <entt/entt.hpp>

#include "core/events.hpp"
#include "graphics/events.hpp"
#include "physics/components.hpp"
#include "world/components.hpp"
#include "world/systems.hpp"

void WorldSystem::init(entt::registry& reg) {
    subscribe_local_event<TileMapComp, RenderEvent, &TileMapComp::OnRender>(reg);
}

void TileMapComp::OnRender(RenderEvent& ev) {
    sf::RenderStates states;
    states.texture = texture;
    ev.window->draw(vertices, states);
}

namespace MapUtil {

void rebuildMesh(entt::entity ent, TileMapComp& map, entt::registry& reg) {
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

            float u = type.x * ts;
            float v = type.y * ts;

            quad[0].texCoords = {u, v};
            quad[1].texCoords = {u + ts, v};
            quad[2].texCoords = {u, v + ts};

            quad[3].texCoords = {u, v + ts};
            quad[4].texCoords = {u + ts, v};
            quad[5].texCoords = {u + ts, v + ts};

            for (int i = 0; i < 6; ++i) quad[i].color = sf::Color::White;
        }
    }

    if (BoundsComp* comp = reg.try_get<BoundsComp>(ent)) {
        comp->bounds = {{0.f, 0.f}, {map.width * map.tileSize, map.height * map.tileSize}};
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
        return {(uint8_t)-1, (uint8_t)-1}; // invalid

    return map.grid[tilePos.x + tilePos.y * map.width];
}

}
