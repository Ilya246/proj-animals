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
}

void ButtonComp::OnClick(ClickEvent& ev) {
    exec(ev);
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
