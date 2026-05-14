#include <algorithm>
#include <SFML/Graphics/Rect.hpp>
#include "core/events.hpp"
#include "physics/components.hpp"
#include "ui/components.hpp"

LayoutConstraints query_child_constraints(entt::registry& reg, entt::entity child) {
    UIComp& ui = reg.get<UIComp>(child);
    if (ui._cachedConstraints) return *ui._cachedConstraints;

    LayoutConstraints c;
    UIQueryChildEvent qEv{c};
    raise_local_event(reg, child, qEv);

    ui._cachedConstraints = c;
    return c;
}

std::vector<entt::entity> get_visible_children(const std::vector<entt::entity>& from, entt::registry& reg) {
    std::vector<entt::entity> ret;
    for (entt::entity child : from) {
        if (UIComp* ui = reg.try_get<UIComp>(child))
            if (!ui->_parentHidden && !ui->selfHidden)
                ret.push_back(child);
    }
    return ret;
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity ent, UIFullAllocatorComp& comp, entt::registry& reg) {
    sf::FloatRect newBounds = apply_dynamic_bounds(ev.newBounds, comp.bounds);

    UIComp& ui = reg.get<UIComp>(ent);
    ui.allocatedBounds = newBounds;
    newBounds.position += ui.childOffset;

    const std::vector<entt::entity> children = get_visible_children(ui.children, reg);
    for (entt::entity child : children) {
        raise_local_event(reg, child, UISizeAllocatedEvent{newBounds});
    }
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity ent, UILayoutComp& comp, entt::registry& reg) {
    sf::FloatRect newBounds = apply_dynamic_bounds(ev.newBounds, comp.bounds);

     if (newBounds.size.x <= 0.f || newBounds.size.y <= 0.f) return;

    UIComp& ui = reg.get<UIComp>(ent);
    const std::vector<entt::entity> children = get_visible_children(ui.children, reg);
    float borderInset = 0.f;
    if (UIRectComp* rect = reg.try_get<UIRectComp>(ent)) borderInset = rect->borderThickness;
    float paddingTotal = borderInset + comp.padding;

    if (children.empty()) {
        ui.allocatedBounds = newBounds;
        reg.get<BoundsComp>(ent).bounds = sf::FloatRect{zeroVec, newBounds.size};
        reg.get<PositionComp>(ent).position = newBounds.position;
        return;
    }

    std::vector<LayoutConstraints> childConstraints;
    childConstraints.reserve(children.size());
    float sumMinMain = 0.f, sumExpandWeight = 0.f, maxMinCross = 0.f;
    bool anyExpandCross = false;

    for (entt::entity child : children) {
        LayoutConstraints c = query_child_constraints(reg, child);
        childConstraints.push_back(c);
        if (comp.mode == UILayoutMode::Vertical) {
            sumMinMain += c.minHeight;
            if (c.expandY) sumExpandWeight += c.weightY;
            maxMinCross = std::max(maxMinCross, c.minWidth);
            if (c.expandX) anyExpandCross = true;
        } else {
            sumMinMain += c.minWidth;
            if (c.expandX) sumExpandWeight += c.weightX;
            maxMinCross = std::max(maxMinCross, c.minHeight);
            if (c.expandY) anyExpandCross = true;
        }
    }

    float totalSpacing = comp.spacing * (children.size() - 1);
    float totalNeededMain = sumMinMain + totalSpacing;

    float maxAvailableMain = (comp.mode == UILayoutMode::Vertical) ? newBounds.size.y - paddingTotal * 2.f : newBounds.size.x - paddingTotal * 2.f;
    float maxAvailableCross = (comp.mode == UILayoutMode::Vertical) ? newBounds.size.x - paddingTotal * 2.f : newBounds.size.y - paddingTotal * 2.f;

    if (comp.allowOverflow) {
        maxAvailableMain = std::max(maxAvailableMain, totalNeededMain);
        sumExpandWeight = 0.f; // Ignore expansion requests in main axis if we're configured to overflow instead
    }

    float finalContentMain = sumExpandWeight > 0.f ? maxAvailableMain : std::min(maxAvailableMain, totalNeededMain);
    float finalContentCross = anyExpandCross ? maxAvailableCross : std::min(maxAvailableCross, maxMinCross);

    float spareMain = finalContentMain - totalNeededMain;
    float currentMain = paddingTotal;
    float contentCrossStart = paddingTotal;

    float extraMain = maxAvailableMain - finalContentMain;
    if (comp.invertAnchor && extraMain > 0.f)
        currentMain += extraMain;

    for (size_t i = 0; i < children.size(); ++i) {
        entt::entity child = children[i];
        LayoutConstraints& c = childConstraints[i];

        float cWidth, cHeight;
        if (comp.mode == UILayoutMode::Vertical) {
            cWidth = c.expandX ? finalContentCross : std::min(finalContentCross, c.minWidth);
            float myHeight = c.minHeight;
            if (spareMain < 0.f && sumMinMain > 0.f) myHeight += spareMain * (c.minHeight / sumMinMain);
            else if (spareMain > 0.f && c.expandY && sumExpandWeight > 0.f) myHeight += spareMain * (c.weightY / sumExpandWeight);
            cHeight = std::max(0.f, myHeight);

            sf::Vector2f pos = sf::Vector2f(contentCrossStart, currentMain) + ui.childOffset + newBounds.position;
            raise_local_event(reg, child, UISizeAllocatedEvent{sf::FloatRect{pos, {cWidth, cHeight}}});
            currentMain += cHeight + comp.spacing;
        } else {
            cHeight = c.expandY ? finalContentCross : std::min(finalContentCross, c.minHeight);
            float myWidth = c.minWidth;
            if (spareMain < 0.f && sumMinMain > 0.f) myWidth += spareMain * (c.minWidth / sumMinMain);
            else if (spareMain > 0.f && c.expandX && sumExpandWeight > 0.f) myWidth += spareMain * (c.weightX / sumExpandWeight);
            cWidth = std::max(0.f, myWidth);

            sf::Vector2f pos = sf::Vector2f(currentMain, contentCrossStart) + ui.childOffset + newBounds.position;
            raise_local_event(reg, child, UISizeAllocatedEvent{sf::FloatRect{pos, {cWidth, cHeight}}});
            currentMain += cWidth + comp.spacing;
         }
     }

    float finalTotalWidth = (comp.mode == UILayoutMode::Vertical) ? finalContentCross + paddingTotal * 2.f : finalContentMain + paddingTotal * 2.f;
    float finalTotalHeight = (comp.mode == UILayoutMode::Vertical) ? finalContentMain + paddingTotal * 2.f : finalContentCross + paddingTotal * 2.f;

    sf::FloatRect finalBounds{newBounds.position, {finalTotalWidth, finalTotalHeight}};
    ui.allocatedBounds = finalBounds;
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity ent, UITileLayoutComp& comp, entt::registry& reg) {
    sf::FloatRect newBounds = apply_dynamic_bounds(ev.newBounds, comp.bounds);
    if (newBounds.size.x <= 0.f || newBounds.size.y <= 0.f) return;

    float borderTop = 0.f, borderBottom = 0.f;
    float borderLeft = 0.f, borderRight = 0.f;
    if (UIRectComp* rect = reg.try_get<UIRectComp>(ent)) {
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
    const std::vector<entt::entity> children = get_visible_children(ui.children, reg);

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
    ev.handled = true;
}

template<>
void handle_event(UIQueryChildEvent& ev, entt::entity, UITileLayoutComp&, entt::registry&) {
    ev.constraints.expandX = true;
    ev.constraints.expandY = true;
}

template<>
void handle_event(UIQueryChildEvent& ev, entt::entity ent, UILayoutComp& comp, entt::registry& reg) {
    UIComp& ui = reg.get<UIComp>(ent);
    float totalMinWidth = 0.f, totalMinHeight = 0.f;
    bool expandX = false, expandY = false;

    int childCount = 0;
    for (entt::entity child : get_visible_children(ui.children, reg)) {
        LayoutConstraints c = query_child_constraints(reg, child);
        if (comp.mode == UILayoutMode::Vertical) {
            totalMinHeight += c.minHeight;
            totalMinWidth = std::max(totalMinWidth, c.minWidth);
        } else {
            totalMinWidth += c.minWidth;
            totalMinHeight = std::max(totalMinHeight, c.minHeight);
        }
        expandX = expandX || c.expandX;
        expandY = expandY || c.expandY;
        childCount++;
    }
    if (childCount > 0) {
        if (comp.mode == UILayoutMode::Vertical) totalMinHeight += comp.spacing * (childCount - 1);
        else totalMinWidth += comp.spacing * (childCount - 1);
    }
    totalMinWidth += comp.padding * 2.f;
    totalMinHeight += comp.padding * 2.f;

    if (UIRectComp* rect = reg.try_get<UIRectComp>(ent)) {
        totalMinWidth += rect->borderThickness * 2.f;
        totalMinHeight += rect->borderThickness * 2.f;
    }

    ev.constraints.minWidth = std::max(ev.constraints.minWidth, totalMinWidth);
    ev.constraints.minHeight = std::max(ev.constraints.minHeight, totalMinHeight);
    ev.constraints.expandX = ev.constraints.expandX || expandX;
    ev.constraints.expandY = ev.constraints.expandY || expandY;
}

template<>
void handle_event(UIQueryChildEvent& ev, entt::entity ent, UIFullAllocatorComp&, entt::registry& reg) {
    UIComp& ui = reg.get<UIComp>(ent);
    float maxMinWidth = 0.f, maxMinHeight = 0.f;
    bool expandX = false, expandY = false;
    for (entt::entity child : get_visible_children(ui.children, reg)) {
        LayoutConstraints c = query_child_constraints(reg, child);
        maxMinWidth = std::max(maxMinWidth, c.minWidth);
        maxMinHeight = std::max(maxMinHeight, c.minHeight);
        expandX = expandX || c.expandX;
        expandY = expandY || c.expandY;
    }

    ev.constraints.minWidth = std::max(ev.constraints.minWidth, maxMinWidth);
    ev.constraints.minHeight = std::max(ev.constraints.minHeight, maxMinHeight);
    ev.constraints.expandX = ev.constraints.expandX || expandX;
    ev.constraints.expandY = ev.constraints.expandY || expandY;
}
