#pragma once
#include <SFML/Graphics/Text.hpp>
#include <functional>

#include "entt/entity/fwd.hpp"
#include "graphics/events.hpp"
#include "input/events.hpp"
#include "physics/events.hpp"
#include "serialization/serialization.hpp"
#include "ui/events.hpp"

// Automatically sizes our BoundsComp to match screen as seen by given camera
struct UIScreenComp {
    entt::entity camera;
};

// Causes us and all our children to not draw
struct UIHiddenComp {
    void OnTryDraw(ShouldRenderEvent&);

    bool hidden = true;
};

///
/// Allocators
///

// On resize, allocates all its available space to children.
struct UIFullAllocatorComp {
    void OnResize(BoundsResizeEvent&);

    std::vector<entt::entity> children = {};
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
    std::vector<entt::entity> children = {};
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

    // Ratio of parent's size to take up
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

// run code on click
struct ButtonComp {
    std::function<void(ClickEvent& ev)> exec;

    void OnClick(ClickEvent&);
};

struct TextComp {
    void OnRender(RenderEvent&);
    void OnResize(BoundsResizeEvent&);

    sf::Text text;
    bool wrap = true;
    std::string originalString = "";

    REGISTER_SERIALIZABLE(TextComp, Text)
};
