#pragma once
#include <entt/entity/fwd.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "graphics/events.hpp"
#include "serialization/serialization.hpp"

enum ZLevel : int32_t {
    // First 26 bits - offset
    // Last 6 bits - layer
    // Entities can have differing offset in the same layer but still be guaranteed to render under upper layers
    z_step = 1 << 26,
    z_world = 0, // world
    z_entity = z_step, // entities
    z_ui = z_step * 16, // UI
};

struct RenderableComp {
    int32_t zLevel = z_entity;
    // Bounds for culling
    std::optional<sf::FloatRect> bounds{};

    REGISTER_SERIALIZABLE(RenderableComp, Renderable)
};

// Applies stencil to us during draw
struct StencilDrawComp {
    std::optional<sf::FloatRect> bounds{};

    REGISTER_SERIALIZABLE(StencilDrawComp, UIStencil)
};

// Renders entities with the same world parent in bounds. 
struct CameraComp {
    float scale;
    int32_t zLevel; // Camera's own z-level. Takes priority over renderable z-level.
    sf::View view{};

    REGISTER_SERIALIZABLE(CameraComp, Camera)
};

struct SpriteComp {
    sf::Sprite sprite;

    void OnRender(RenderEvent&);

    REGISTER_SERIALIZABLE(SpriteComp, Sprite)
};
