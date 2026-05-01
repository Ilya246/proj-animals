#pragma once
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <functional>

#include "entt/entity/fwd.hpp"
#include "graphics/events.hpp"
#include "input/events.hpp"
#include "physics/events.hpp"
#include "serialization/serialization.hpp"
#include "ui/events.hpp"
#include "utility/constants.hpp"
#include "utility/geom.hpp"

// Automatically sizes our BoundsComp to match the entire screen as seen by given camera.
struct UIScreenComp {
    entt::entity camera;
};

// General UI component holding data relevant to most UIs.
struct UIComp {
    // Whether we want to be hidden ourselves
    bool selfHidden = false;
    // Whether to stencil child UIs within our bounds
    bool isStencil = false;
    std::vector<entt::entity> children;
    // Cached stencil in local coordinates
    std::optional<sf::FloatRect> _cachedStencil = std::nullopt;
    // Cache: whether our parent is hidden
    bool _parentHidden = false;

    // Offset applied to children's positions during allocation.
    sf::Vector2f childOffset = zeroVec;
    // Encloses all space we have allocated to children. Does not respect child_offset.
    std::optional<sf::FloatRect> allocatedBounds;

    void OnTryDraw(ShouldRenderEvent&);
    void OnPropagate(UIPropagateEvent&);
    void OnResize(BoundsResizeEvent&);

    void set_hidden(bool hide, entt::registry& reg, entt::entity ent);
    void assign_stencil(std::optional<sf::FloatRect> stencil, entt::registry& reg, entt::entity ent);
    void propagate(UIPropagateEvent& propagate, entt::entity ent, entt::registry& reg);
};

///
/// Allocators
///

// On resize, allocates all its available space to children.
// Supposed to be used with UIAnchorComp or similar.
struct UIFullAllocatorComp {
    void OnResize(BoundsResizeEvent&);

    std::optional<DynamicBounds> bounds = {};
};

// Layout modes for UILayoutComp and similar.
enum class UILayoutMode : uint8_t {
    Horizontal,
    Vertical
};

// Evenly lays out child entities horizontally or vertically.
struct UILayoutComp {
    void OnResize(BoundsResizeEvent&);

    UILayoutMode mode = UILayoutMode::Vertical;
    float padding = 0.f; // inner padding around all children
    float spacing = 0.f; // gap between adjacent children
    std::optional<DynamicBounds> bounds = {};
};

// Lays out child entities in tiled rows.
struct UITileLayoutComp {
    void OnResize(BoundsResizeEvent&);

    sf::Vector2f tileSize;
    float padding = 0.f;
    float spacing = 0.f;
    std::optional<DynamicBounds> bounds = {};
};

///
/// Modifiers
///

// Grants this UI a scrollbar. Usable if not all of our children can fit into bounds.
struct UIScrollAreaComp {
    float scrollFraction = 1.f; // 1.f = top, 0.f = bottom

    void OnScroll(ScrollEvent&);
};

///
/// Allocation handlers
///

// Makes this UI fill all allocated space.
struct UIFillComp {
    void OnAllocate(UISizeAllocatedEvent&);

    bool _dummy = true; // needed for EnTT
};

// Makes this UI anchor to some position in allocated space.
struct UIAnchorComp {
    void OnAllocate(UISizeAllocatedEvent&);

    DynamicBounds bounds;
};

// Makes this UI ignore allocated space and take a fixed size.
struct UIAbsoluteBoundsComp {
    void OnAllocate(UISizeAllocatedEvent&);

    // Absolute bounds
    sf::FloatRect bounds;
};

///
/// Elements
///

// Renders us as a filled rectangle with borders.
struct UIRectComp {
    void OnRender(RenderEvent&);

    sf::Color fillColor = sf::Color::Transparent;
    sf::Color borderColor = sf::Color::White;
    float borderThickness = 1.f;
};

// Similar to UIRectComp, but has a header.
// TODO: simplify, tie into UIRectComp.
struct UIWindowComp {
    void OnRender(RenderEvent&);

    sf::Color fillColor = sf::Color::Transparent;
    sf::Color borderColor = sf::Color::White;
    sf::Color headerColor = sf::Color::Blue;
    float borderThickness = 1.f;
    float headerHeight = 20.f;
};

// Can be dragged with the mouse if clicked and held within bounds.
struct DraggableComp {
    void OnClick(ClickEvent&);

    std::optional<DynamicBounds> bounds = {};
    bool being_dragged = false;
    sf::Vector2f anchorCoords = {0.f, 0.f};
};

// Runs a function when clicked.
struct ButtonComp {
    std::function<void(ClickEvent& ev)> exec;

    void OnClick(ClickEvent&);
};

// Hides or unhides when a keyboard key is pressed.
struct ButtonToggledUIComp {
    sf::Keyboard::Key triggerKey;
};

// Displays text.
struct TextComp {
    void OnRender(RenderEvent&);
    void OnResize(BoundsResizeEvent&);

    sf::Text text;
    bool wrap = true;
    std::string originalString = "";

    REGISTER_SERIALIZABLE(TextComp, Text)
};
