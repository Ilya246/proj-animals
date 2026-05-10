#pragma once
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <functional>

#include "core/events.hpp"
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
    DynamicBounds stencilArea = DynamicBounds::full;

    std::vector<entt::entity> children;
    // Cached stencil in local coordinates
    std::optional<sf::FloatRect> _cachedStencil = {};
    // Cache: whether our parent is hidden
    bool _parentHidden = false;

    // Offset applied to children's positions during allocation.
    sf::Vector2f childOffset = zeroVec;
    // Encloses all space we have allocated to children. Does not respect child_offset.
    std::optional<sf::FloatRect> allocatedBounds;

    HANDLE_EVENT(UIComp, ShouldRenderEvent)
    HANDLE_EVENT(UIComp, UIPropagateEvent)
    HANDLE_EVENT(UIComp, BoundsResizeEvent)

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
    DynamicBounds bounds = DynamicBounds::full;

    HANDLE_EVENT(UIFullAllocatorComp, BoundsResizeEvent)
};

// Layout modes for UILayoutComp and similar.
enum class UILayoutMode : uint8_t {
    Horizontal,
    Vertical
};

// Evenly lays out child entities horizontally or vertically.
struct UILayoutComp {
    UILayoutMode mode = UILayoutMode::Vertical;
    float padding = 0.f; // inner padding around all children
    float spacing = 0.f; // gap between adjacent children
    DynamicBounds bounds = DynamicBounds::full;

    HANDLE_EVENT(UILayoutComp, BoundsResizeEvent)
};

// Lays out child entities in tiled rows.
struct UITileLayoutComp {
    sf::Vector2f tileSize;
    float padding = 0.f;
    float spacing = 0.f;
    DynamicBounds bounds = DynamicBounds::full;

    HANDLE_EVENT(UITileLayoutComp, BoundsResizeEvent)
};

///
/// Modifiers
///

// Grants this UI a scrollbar. Usable if not all of our children can fit into bounds and we have a stencil (otherwise a scrollbar does not make sense).
struct UIScrollAreaComp {
    DynamicBounds bounds; // Bounds to render the scrollbar in.
    sf::Color innerColor = sf::Color::Blue; // Color of the area our scrollbar moves in.
    sf::Color outlineColor = sf::Color::White; // Outline color of that same area.
    sf::Color barColor = sf::Color::Black; // Color of the scrollbar itself.
    float outlineThickness = 1.f;

    float scrollMul = 25.f;
    float scrollPos = 0.f; // 0.f = top, Y-down

    HANDLE_EVENT(UIScrollAreaComp, ScrollEvent)
    HANDLE_EVENT(UIScrollAreaComp, RenderEvent)
};

///
/// Allocation handlers
///

// Makes this UI fill all allocated space.
struct UIFillComp {
    bool _dummy = true; // needed for EnTT

    HANDLE_EVENT(UIFillComp, UISizeAllocatedEvent)
};

// Makes this UI anchor to some position in allocated space.
struct UIAnchorComp {
    DynamicBounds bounds;

    HANDLE_EVENT(UIAnchorComp, UISizeAllocatedEvent)
};

// Makes this UI ignore allocated space and take a fixed size.
struct UIAbsoluteBoundsComp {
    // Absolute bounds
    sf::FloatRect bounds;

    HANDLE_EVENT(UIAbsoluteBoundsComp, UISizeAllocatedEvent)
};

///
/// Elements
///

// Renders us as a filled rectangle with borders.
struct UIRectComp {
    sf::Color fillColor = sf::Color::Transparent;
    sf::Color borderColor = sf::Color::White;
    float borderThickness = 1.f;

    HANDLE_EVENT(UIRectComp, RenderEvent)
};

// Similar to UIRectComp, but has a header.
// TODO: simplify, tie into UIRectComp.
struct UIWindowComp {
    sf::Color fillColor = sf::Color::Transparent;
    sf::Color borderColor = sf::Color::White;
    sf::Color headerColor = sf::Color::Blue;
    float borderThickness = 1.f;
    float headerHeight = 20.f;

    HANDLE_EVENT(UIWindowComp, RenderEvent)
};

// Can be dragged with the mouse if clicked and held within bounds.
struct DraggableComp {
    std::optional<DynamicBounds> bounds = {};
    bool being_dragged = false;
    sf::Vector2f anchorCoords = {0.f, 0.f};

    HANDLE_EVENT(DraggableComp, ClickEvent)
};

// Runs a function when clicked.
struct ButtonComp {
    std::function<void(ClickEvent&, entt::entity, entt::registry&)> exec;

    HANDLE_EVENT(ButtonComp, ClickEvent)
};

// Hides or unhides when a keyboard key is pressed.
struct ButtonToggledUIComp {
    sf::Keyboard::Key triggerKey;
};

// Displays text.
struct TextComp {
    sf::Text text;
    bool wrap = true;
    std::string originalString = "";

    HANDLE_EVENT(TextComp, RenderEvent)
    HANDLE_EVENT(TextComp, BoundsResizeEvent)

    REGISTER_SERIALIZABLE(TextComp, Text)
};

struct ToggleButtonComp {
    bool isToggled = false;
    std::function<void(bool, entt::entity, entt::registry&)> cb;
    std::vector<entt::entity> exclusiveGroup;
    sf::Color onColor;
    sf::Color offColor;

    HANDLE_EVENT(ToggleButtonComp, ClickEvent)
};

struct TooltipProviderComp {
    std::string text;
    float threshold = 0.5f;
    entt::entity spawnedTooltip = entt::null;
};
