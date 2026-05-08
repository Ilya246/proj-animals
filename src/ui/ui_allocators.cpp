#include <algorithm>
#include <SFML/Graphics/Rect.hpp>
#include "core/events.hpp"
#include "physics/components.hpp"
#include "ui/components.hpp"

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity ent, UIFullAllocatorComp& comp, entt::registry& reg) {
    sf::FloatRect newBounds = apply_dynamic_bounds(ev.newBounds, comp.bounds);

    UIComp& ui = reg.get<UIComp>(ent);
    ui.allocatedBounds = newBounds;
    newBounds.position += ui.childOffset;

    const std::vector<entt::entity>& children = ui.children;
    for (entt::entity child : children) {
        raise_local_event(reg, child, UISizeAllocatedEvent{newBounds});
    }
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity ent, UILayoutComp& comp, entt::registry& reg) {
    sf::FloatRect newBounds = apply_dynamic_bounds(ev.newBounds, comp.bounds);

    if (newBounds.size.x <= 0.f || newBounds.size.y <= 0.f) return;

    // Account for UIRectComp border insets
    float borderTop = 0.f, borderBottom = 0.f;
    float borderLeft = 0.f, borderRight = 0.f;
    if (auto* rect = reg.try_get<UIRectComp>(ent)) {
        float bt = rect->borderThickness;
        borderTop = borderBottom = borderLeft = borderRight = bt;
    }

    // Content rectangle (all in local pos-coordinates, Y-up)
    const float contentLeft   = newBounds.position.x + borderLeft + comp.padding;
    const float contentBottom = newBounds.position.y + borderBottom + comp.padding;
    const float contentRight  = newBounds.position.x + newBounds.size.x - borderRight - comp.padding;
    const float contentTop    = newBounds.position.y + newBounds.size.y - borderTop - comp.padding;
    const float contentWidth  = contentRight - contentLeft;
    const float contentHeight = contentTop - contentBottom;

    if (contentWidth <= 0.f || contentHeight <= 0.f) return;

    UIComp& ui = reg.get<UIComp>(ent);
    const std::vector<entt::entity>& children = ui.children;
    ui.allocatedBounds = {{contentLeft, contentBottom}, {contentWidth, contentHeight}};

    if (children.empty()) return;

    // TODO: compact this somehow? Vector addition?
    // down-up
    if (comp.mode == UILayoutMode::Vertical) {
        float fillHeight = contentHeight / children.size();
        float totalSpacing = comp.spacing * (children.size() - 1.f);
        float avgSpacing = totalSpacing / children.size();
        float cursorY = contentBottom; // pos-coords: top = high Y

        for (entt::entity child : children) {
            float childWidth = contentWidth;
            float childHeight = fillHeight - avgSpacing;

            sf::Vector2f pos = sf::Vector2f(contentLeft, cursorY) + ui.childOffset;
            sf::FloatRect bounds = sf::FloatRect{pos, {childWidth, childHeight}};
            raise_local_event(reg, child, UISizeAllocatedEvent{bounds});

            cursorY += fillHeight;
        }
    // left-right
    } else {
        float fillWidth = contentWidth / children.size();
        float totalSpacing = comp.spacing * (children.size() - 1.f);
        float avgSpacing = totalSpacing / children.size();
        float cursorX = contentLeft;

        for (entt::entity child : children) {
            float childHeight = contentHeight;
            float childWidth = fillWidth - avgSpacing;

            sf::Vector2f pos = sf::Vector2f(cursorX, contentBottom) + ui.childOffset;
            sf::FloatRect bounds = sf::FloatRect{pos, {childWidth, childHeight}};
            raise_local_event(reg, child, UISizeAllocatedEvent{bounds});

            cursorX += fillWidth;
        }
    }
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity ent, UITileLayoutComp& comp, entt::registry& reg) {
    sf::FloatRect newBounds = apply_dynamic_bounds(ev.newBounds, comp.bounds);
    if (newBounds.size.x <= 0.f || newBounds.size.y <= 0.f) return;

    float borderTop = 0.f, borderBottom = 0.f;
    float borderLeft = 0.f, borderRight = 0.f;
    if (auto* rect = reg.try_get<UIRectComp>(ent)) {
        float bt = rect->borderThickness;
        borderTop = borderBottom = borderLeft = borderRight = bt;
    }

    // Content rectangle (all in local pos-coordinates, Y-up)
    const float contentLeft   = newBounds.position.x + borderLeft + comp.padding;
    const float contentBottom = newBounds.position.y + borderBottom + comp.padding;
    const float contentRight  = newBounds.position.x + newBounds.size.x - borderRight - comp.padding;
    const float contentTop    = newBounds.position.y + newBounds.size.y - borderTop - comp.padding;
    const float contentWidth  = contentRight - contentLeft;
    const float contentHeight = contentTop - contentBottom;
    sf::FloatRect contentRect {{contentLeft, contentBottom}, {contentWidth, contentHeight}};

    UIComp& ui = reg.get<UIComp>(ent);
    const std::vector<entt::entity>& children = ui.children;

    if (children.empty()) return;

    int cols = std::max(1, (int)((contentWidth + comp.spacing) / (comp.tileSize.x + comp.spacing)));

    int col = 0;
    float cursorX = contentLeft;
    float cursorY = contentTop - comp.tileSize.y;
    float minY = cursorY;
    float maxX = cursorX;

    for (entt::entity child : children) {
        sf::Vector2f pos(cursorX, cursorY);
        sf::FloatRect cBounds(pos + ui.childOffset, comp.tileSize);
        raise_local_event(reg, child, UISizeAllocatedEvent{cBounds});

        col++;
        if (col >= cols) {
            col = 0;
            cursorX = contentLeft;
            cursorY -= (comp.tileSize.y + comp.spacing);
        } else {
            cursorX += (comp.tileSize.x + comp.spacing);
        }
        minY = std::min(minY, cursorY);
        maxX = std::max(maxX, cursorX);
    }

    ui.allocatedBounds = {{contentLeft, minY}, {maxX + comp.tileSize.x - contentLeft, contentTop - minY}};
}

template<>
void handle_event(ScrollEvent& ev, entt::entity ent, UIScrollAreaComp& comp, entt::registry& reg) {
    UIComp& ui = reg.get<UIComp>(ent);
    if (!ui.allocatedBounds || !ui.isStencil || !ui._cachedStencil)
        return;

    sf::FloatRect availableBounds = *ui._cachedStencil;
    sf::FloatRect allocatedBounds = *ui.allocatedBounds;

    float scrollMaxY = allocatedBounds.size.y - availableBounds.size.y;

    comp.scrollPos = std::clamp(comp.scrollPos - ev.delta * comp.scrollMul, 0.f, scrollMaxY);

    ui.childOffset.y = comp.scrollPos;
    BoundsComp bounds = reg.get<BoundsComp>(ent);
    bounds.resize(bounds.bounds, ent, reg);
    *ev.handled = true;
}
