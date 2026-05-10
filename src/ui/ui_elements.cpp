#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/components.hpp"

template<>
void handle_event(RenderEvent& ev, entt::entity ent, UIRectComp& comp, entt::registry& reg) {
    BoundsComp& bounds = reg.get<BoundsComp>(ent);

    sf::Vector2f worldPos = Physics::getWorldPos(ent, reg);

    // Bottom-left and top-right in pos-coords (Y-up)
    sf::Vector2f bl = worldPos + bounds.bounds.position;
    sf::Vector2f tr = bl + bounds.bounds.size;

    // Convert to draw-coords
    bl.y *= -1;
    tr.y *= -1;
    sf::Vector2f drawSize(tr.x - bl.x - comp.borderThickness * 2, bl.y - tr.y  - comp.borderThickness * 2);
    sf::Vector2f drawPos(bl.x + comp.borderThickness, tr.y + comp.borderThickness);

    sf::RectangleShape rect(drawSize);
    rect.setPosition(drawPos);
    rect.setFillColor(comp.fillColor);
    rect.setOutlineColor(comp.borderColor);
    rect.setOutlineThickness(comp.borderThickness);
    ev.window.draw(rect);
}

template<>
void handle_event(ClickEvent& ev, entt::entity ent, DraggableComp& comp, entt::registry& reg) {
    bool pressed = ev.pressed;
    // check bounds
    if (comp.bounds) {
        if (auto boundsComp = reg.try_get<BoundsComp>(ent)) {
            sf::FloatRect scaledBounds = apply_dynamic_bounds(boundsComp->bounds, *comp.bounds);
            pressed &= scaledBounds.contains(ev.relativeCoords);
        }
    }

    comp.being_dragged = pressed;
    comp.anchorCoords = ev.relativeCoords;
    ev.handled = true;
}

template<>
void handle_event(ClickEvent& ev, entt::entity ent, ButtonComp& comp, entt::registry& reg) {
    if (!ev.pressed) return;
    comp.exec(ev, ent, reg);
    ev.handled = true;
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

void set_text(sf::FloatRect bounds, TextComp& comp) {
    if (!comp.wrap) return;

    // Capture the original string the first time we wrap
    if (comp.originalString.empty())
        comp.originalString = comp.text.getString();

    if (comp.originalString.empty()) return;

    if (bounds.size.x <= 0.f) return;

    float maxWidth = bounds.size.x;

    wrapText(comp.originalString, comp.text, maxWidth);
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity, TextComp& comp, entt::registry&) {
    set_text(ev.newBounds, comp);
}

template<>
void handle_event(UIQueryChildEvent& ev, entt::entity ent, TextComp& comp, entt::registry& reg) {
    sf::FloatRect bSize = reg.get<BoundsComp>(ent).bounds;
    if (bSize.size.x <= 0.f)
        bSize = sf::FloatRect{{0.f, 0.f}, {2e16f, 2e16f}};
    set_text(bSize, comp);
    sf::FloatRect bounds = comp.text.getLocalBounds();
    ev.constraints.minWidth = std::max(ev.constraints.minWidth, bounds.position.x + bounds.size.x);
    ev.constraints.minHeight = std::max(ev.constraints.minHeight, (float)comp.text.getCharacterSize());
}

template<>
void handle_event(RenderEvent& ev, entt::entity ent, TextComp& comp, entt::registry& reg) {
    sf::Vector2f pos = Physics::getWorldPos(ent, reg);
    if (auto* boundsComp = reg.try_get<BoundsComp>(ent)) {
        pos += boundsComp->bounds.position;
        pos.y += boundsComp->bounds.size.y;
    }
    pos.y *= -1.f; // convert to draw-coordinates
    comp.text.setPosition(pos);
    ev.window.draw(comp.text);
}

template<>
void handle_event(RenderEvent& ev, entt::entity ent, UIScrollAreaComp& comp, entt::registry& reg) {
    const UIComp& ui = reg.get<UIComp>(ent);
    if (!ui.allocatedBounds || !ui.isStencil || !ui._cachedStencil)
        return;

    const BoundsComp& boundsComp = reg.get<BoundsComp>(ent);
    sf::FloatRect totalBounds = apply_dynamic_bounds(boundsComp.bounds, comp.bounds);

    sf::FloatRect availableBounds = *ui._cachedStencil;
    sf::FloatRect allocatedBounds = *ui.allocatedBounds;

    // ratio represents the height of our scrollbar relative to the total allocated height,
    // so we can scroll up until y + ratio = 1, or y = 1 - ratio
    float ratio = availableBounds.size.y / allocatedBounds.size.y;
    float scrollMaxY = 1.f - ratio;
    float scrollPosMax = allocatedBounds.size.y - availableBounds.size.y;
    float scrollFraction = 1.f - comp.scrollPos / scrollPosMax;

    sf::Vector2f worldPos = Physics::getWorldPos(ent, reg);
    sf::Vector2f outlineVec {comp.outlineThickness, comp.outlineThickness};
    sf::FloatRect innerBounds = {totalBounds.position + outlineVec, totalBounds.size - outlineVec * 2.f};

    sf::Vector2f rectPos = innerBounds.position + worldPos;
    rectPos.y *= -1.f;
    sf::Vector2f rectSize = innerBounds.size;
    rectSize.y *= -1.f;
    sf::RectangleShape rect(rectSize);
    rect.setPosition(rectPos);
    rect.setFillColor(comp.innerColor);
    rect.setOutlineColor(comp.outlineColor);
    rect.setOutlineThickness(comp.outlineThickness);
    ev.window.draw(rect);

    float scrollY = scrollMaxY * innerBounds.size.y * scrollFraction;
    float scrollSizeY = innerBounds.size.y * ratio;
    sf::Vector2f scrollSize = {innerBounds.size.x, -scrollSizeY};
    sf::Vector2f barPos = sf::Vector2f{innerBounds.position.x, scrollY} + worldPos;
    barPos.y *= -1.f;
    sf::RectangleShape bar(scrollSize);
    bar.setPosition(barPos);
    bar.setFillColor(comp.barColor);
    bar.setOutlineThickness(0.f);
    ev.window.draw(bar);
}

template<>
void handle_event(ClickEvent& ev, entt::entity ent, ToggleButtonComp& comp, entt::registry& reg) {
    if (!ev.pressed) return;

    comp.isToggled = !comp.isToggled;
    if (auto* rect = reg.try_get<UIRectComp>(ent)) {
        rect->fillColor = comp.isToggled ? comp.onColor : comp.offColor;
    }
    if (comp.cb) comp.cb(comp.isToggled, ent, reg);

    if (comp.isToggled) {
        for (auto exEnt : comp.exclusiveGroup) {
            if (reg.valid(exEnt) && exEnt != ent) {
                if (auto* exComp = reg.try_get<ToggleButtonComp>(exEnt)) {
                    if (exComp->isToggled) {
                        exComp->isToggled = false;
                        if (auto* exRect = reg.try_get<UIRectComp>(exEnt)) {
                            exRect->fillColor = exComp->offColor;
                        }
                        if (exComp->cb) exComp->cb(false, exEnt, reg);
                    }
                }
            }
        }
    }
    ev.handled = true;
}
