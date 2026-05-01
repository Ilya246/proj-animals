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
#include "utility/utility.hpp"

// Automatically sizes our BoundsComp to match screen as seen by given camera
struct UIScreenComp {
    entt::entity camera;
};

// Causes us and all our children to not draw
struct UIComp {
    // Whether we want to be hidden ourselves
    bool self_hidden = false;
    // Cache: whether our parent is hidden
    bool parent_hidden = false;
    // Whether to stencil child UIs within our bounds
    bool is_stencil = false;
    // Cached stencil in local coordinates
    std::optional<sf::FloatRect> cached_stencil = std::nullopt;
    std::vector<entt::entity> children;

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
struct UIFullAllocatorComp {
    void OnResize(BoundsResizeEvent&);

    std::optional<DynamicBounds> bounds = {};
};

enum class UILayoutMode : uint8_t {
    Horizontal,
    Vertical
};

// Evenly lays out child entities
struct UILayoutComp {
    void OnResize(BoundsResizeEvent&);

    UILayoutMode mode = UILayoutMode::Vertical;
    float padding = 0.f; // inner padding around all children
    float spacing = 0.f; // gap between adjacent children
    std::optional<DynamicBounds> bounds = {};
};

///
/// Allocation handlers
///

// Makes this UI fill all allocated space
struct UIFillComp {
    void OnAllocate(UISizeAllocatedEvent&);

    bool _dummy = true; // needed for EnTT
};

// Makes this UI anchor to some position in allocated space
struct UIAnchorComp {
    void OnAllocate(UISizeAllocatedEvent&);

    DynamicBounds bounds;
};

// Makes this UI ignore allocated space and take a fixed size
struct UIAbsoluteBoundsComp {
    void OnAllocate(UISizeAllocatedEvent&);

    // Absolute bounds
    sf::FloatRect bounds;
};

///
/// Elements
///

// Renders us as a filled rectangle
struct UIRectComp {
    void OnRender(RenderEvent&);

    sf::Color fillColor = sf::Color::Transparent;
    sf::Color borderColor = sf::Color::White;
    float borderThickness = 1.f;
};

struct UIWindowComp {
    void OnRender(RenderEvent&);

    sf::Color fillColor = sf::Color::Transparent;
    sf::Color borderColor = sf::Color::White;
    sf::Color headerColor = sf::Color::Blue;
    float borderThickness = 1.f;
    float headerHeight = 20.f;
};

struct DraggableComp {
    void OnClick(ClickEvent&);

    std::optional<DynamicBounds> bounds = {};
    bool being_dragged = false;
    sf::Vector2f anchorCoords = {0.f, 0.f};
};

// run code on click
struct ButtonComp {
    std::function<void(ClickEvent& ev)> exec;

    void OnClick(ClickEvent&);
};

struct ButtonToggledUIComp {
    sf::Keyboard::Key triggerKey;
};

struct TextComp {
    void OnRender(RenderEvent&);
    void OnResize(BoundsResizeEvent&);

    sf::Text text;
    bool wrap = true;
    std::string originalString = "";

    REGISTER_SERIALIZABLE(TextComp, Text)
};
