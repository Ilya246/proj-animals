#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/components.hpp"

void UIRectComp::OnRender(RenderEvent& ev) {
    BoundsComp& bounds = ev.reg->get<BoundsComp>(ev.ent);

    sf::Vector2f worldPos = Physics::getWorldPos(ev.ent, *ev.reg);

    // Bottom-left and top-right in pos-coords (Y-up)
    sf::Vector2f bl = worldPos + bounds.bounds.position;
    sf::Vector2f tr = bl + bounds.bounds.size;

    // Convert to draw-coords
    bl.y *= -1;
    tr.y *= -1;
    sf::Vector2f drawSize(tr.x - bl.x - borderThickness * 2, bl.y - tr.y  - borderThickness * 2);
    sf::Vector2f drawPos(bl.x + borderThickness, tr.y + borderThickness);

    sf::RectangleShape rect(drawSize);
    rect.setPosition(drawPos);
    rect.setFillColor(fillColor);
    rect.setOutlineColor(borderColor);
    rect.setOutlineThickness(borderThickness);
    ev.window->draw(rect);
}

void UIWindowComp::OnRender(RenderEvent& ev) {
    BoundsComp& bounds = ev.reg->get<BoundsComp>(ev.ent);
    sf::Vector2f worldPos = Physics::getWorldPos(ev.ent, *ev.reg);

    sf::Vector2f bl = worldPos + bounds.bounds.position;
    sf::Vector2f tr = bl + bounds.bounds.size;
    bl.y *= -1; tr.y *= -1;

    sf::Vector2f drawSize(tr.x - bl.x - borderThickness * 2, bl.y - tr.y  - borderThickness * 2);
    sf::Vector2f drawPos(bl.x + borderThickness, tr.y + borderThickness);
    if (drawSize.x <= 0.f || drawSize.y <= 0.f) return;

    sf::RectangleShape rect(drawSize);
    rect.setPosition(drawPos);
    rect.setFillColor(fillColor);
    rect.setOutlineColor(borderColor);
    rect.setOutlineThickness(borderThickness);
    ev.window->draw(rect);

    if (headerHeight > 0.f) {
        sf::RectangleShape header(sf::Vector2f(drawSize.x, headerHeight));
        header.setPosition(drawPos);
        header.setFillColor(headerColor);
        ev.window->draw(header);
    }
}

void DraggableComp::OnClick(ClickEvent& ev) {
    bool pressed = ev.pressed;
    // check bounds
    if (bounds) {
        if (auto boundsComp = ev.registry->try_get<BoundsComp>(ev.ent)) {
            sf::FloatRect scaledBounds = apply_dynamic_bounds(boundsComp->bounds, *bounds);
            pressed &= scaledBounds.contains(ev.relativeCoords);
        }
    }

    being_dragged = pressed;
    anchorCoords = ev.relativeCoords;
    *ev.handled = true;
}

void ButtonComp::OnClick(ClickEvent& ev) {
    exec(ev);
    *ev.handled = true;
}

size_t wrapText(std::string string, sf::Text& text, float maxWidth) {
    size_t newlines = 0;
    text.setString(string);
    float firstPos = text.findCharacterPos(0).x;
    for (size_t i = 0; i < string.size(); i++) {
        if (text.findCharacterPos(i).x - firstPos > maxWidth) {
            if (i == 0) [[unlikely]] {
                throw std::runtime_error("Couldn't wrap text: width too small.");
            }
            string.insert(i - 1, 1, '\n');
            text.setString(string);
            newlines++;
        }
    }
    return newlines;
}

void TextComp::OnResize(BoundsResizeEvent& ev) {
    if (!wrap) return;

    entt::registry& reg = *ev.registry;
    entt::entity entity = ev.entity;

    // Capture the original string the first time we wrap
    if (originalString.empty())
        originalString = text.getString();

    if (originalString.empty()) return;

    BoundsComp* bounds = reg.try_get<BoundsComp>(entity);
    if (!bounds || bounds->bounds.size.x <= 0.f) return;

    float maxWidth = bounds->bounds.size.x;

    wrapText(originalString, text, maxWidth);
}

void TextComp::OnRender(RenderEvent& ev) {
    sf::Vector2f pos = Physics::getWorldPos(ev.ent, *ev.reg);
    if (auto* boundsComp = ev.reg->try_get<BoundsComp>(ev.ent)) {
        pos += boundsComp->bounds.position;
        pos.y += boundsComp->bounds.size.y;
    }
    pos.y *= -1.f; // convert to draw-coordinates
    text.setPosition(pos);
    ev.window->draw(text);
}

void UIScrollAreaComp::OnRender(RenderEvent& ev) {
    const UIComp& ui = ev.reg->get<UIComp>(ev.ent);
    if (!ui.allocatedBounds || !ui.isStencil || !ui._cachedStencil)
        return;

    const BoundsComp& boundsComp = ev.reg->get<BoundsComp>(ev.ent);
    sf::FloatRect totalBounds = apply_dynamic_bounds(boundsComp.bounds, bounds);

    sf::FloatRect availableBounds = *ui._cachedStencil;
    sf::FloatRect allocatedBounds = *ui.allocatedBounds;

    // ratio represents the height of our scrollbar relative to the total allocated height,
    // so we can scroll up until y + ratio = 1, or y = 1 - ratio
    float ratio = availableBounds.size.y / allocatedBounds.size.y;
    float scrollMaxY = 1.f - ratio;
    float scrollPosMax = allocatedBounds.size.y - availableBounds.size.y;
    float scrollFraction = 1.f - scrollPos / scrollPosMax;

    sf::Vector2f worldPos = Physics::getWorldPos(ev.ent, *ev.reg);
    sf::Vector2f outlineVec {outlineThickness, outlineThickness};
    sf::FloatRect innerBounds = {totalBounds.position + outlineVec, totalBounds.size - outlineVec * 2.f};

    sf::Vector2f rectPos = innerBounds.position + worldPos;
    rectPos.y *= -1.f;
    sf::Vector2f rectSize = innerBounds.size;
    rectSize.y *= -1.f;
    sf::RectangleShape rect(rectSize);
    rect.setPosition(rectPos);
    rect.setFillColor(innerColor);
    rect.setOutlineColor(outlineColor);
    rect.setOutlineThickness(outlineThickness);
    ev.window->draw(rect);

    float scrollY = scrollMaxY * innerBounds.size.y * scrollFraction;
    float scrollSizeY = innerBounds.size.y * ratio;
    sf::Vector2f scrollSize = {innerBounds.size.x, -scrollSizeY};
    sf::Vector2f barPos = sf::Vector2f{innerBounds.position.x, scrollY} + worldPos;
    barPos.y *= -1.f;
    sf::RectangleShape bar(scrollSize);
    bar.setPosition(barPos);
    bar.setFillColor(barColor);
    bar.setOutlineThickness(0.f);
    ev.window->draw(bar);
}
