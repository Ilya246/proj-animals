#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/components.hpp"
#include "ui/system.hpp"

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

static size_t fitText(std::string str, sf::Text& text, float maxWidth) {
    text.setString(str);
    if (maxWidth <= 0.f) return str.size();
    float startX = text.findCharacterPos(0).x;
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '\n') {
            str[i] = ' ';
            text.setString(str);
        }
        if (text.findCharacterPos(i).x - startX > maxWidth) {
            text.setString(str.substr(0, i));
            return str.size() - i;
        }
    }
    return 0;
}

size_t TextBoxComp::getMouseCursorPos(float mouseX) const {
    size_t strsize = text.getString().getSize();
    if (strsize == 0) return 0;

    float lastX = text.findCharacterPos(0).x;
    for (size_t i = 1; i <= strsize; i++) {
        float x = text.findCharacterPos(i).x;
        if (mouseX < (lastX + x) / 2.f) return i - 1;
        lastX = x;
    }
    return strsize;
}

void TextBoxComp::stringChanged(float width) {
    std::string sub;
    if (content.size() > 0) {
        sub = content.substr(viewPos);
        cursorPos = std::min(cursorPos, content.size());
        // cursor should be in [viewPos, viewPos + lastCharsFit]
        size_t offs = lastCharsFit > cursorPos ? 0 : cursorPos - lastCharsFit;
        if (offs != cursorPos)
            viewPos = std::clamp(viewPos, offs, cursorPos);
    } else {
        sub = "";
        cursorPos = 0;
        viewPos = 0;
        text.setString(sub);
        return;
    }

    size_t overflow = fitText(sub, text, width);
    lastCharsFit = content.size() - viewPos - overflow;

    if (cursorPos > viewPos + lastCharsFit) {
        viewPos = cursorPos - lastCharsFit;
        stringChanged(width);
    }
}

void TextBoxComp::eraseSelection(float width) {
    size_t chars = std::max(selectionStart, selectionEnd) - std::min(selectionStart, selectionEnd);
    content.erase(std::min(selectionStart, selectionEnd), chars);
    cursorPos = std::min(cursorPos, std::max(selectionStart, cursorPos > chars ? cursorPos - chars : 0));
    stringChanged(width);
    selectionActive = false;
    selectionStart = std::min(std::max(content.size(), (size_t)1) - 1, selectionStart);
    selectionEnd = std::min(content.size(), selectionEnd);
}

template<>
void handle_event(ClickEvent& ev, entt::entity ent, TextBoxComp& comp, entt::registry& reg) {
    if (ev.button == sf::Mouse::Button::Left) {
        if (ev.pressed) {
            UISystem& sys = reg.ctx().get<UISystem>();
            sys.activeTextbox = ent;

            comp.cursorPos = std::min(comp.viewPos + comp.getMouseCursorPos(ev.worldCoords.x), comp.content.size());
            comp.clickDragged = true;
            comp.selectionStart = comp.cursorPos;
            comp.selectionEnd = comp.cursorPos;
            comp.selectionActive = false;
            ev.handled = true;
        } else {
            if (comp.clickDragged) {
                comp.clickDragged = false;
                if (comp.selectionStart == comp.selectionEnd) {
                    comp.selectionActive = false;
                }
                ev.handled = true;
            }
        }
    }
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity, TextBoxComp& comp, entt::registry&) {
    comp.stringChanged(ev.newBounds.size.x);
}

template<>
void handle_event(UIQueryChildEvent& ev, entt::entity, TextBoxComp& comp, entt::registry&) {
    sf::Vector2f bound = comp.text.getGlobalBounds().size;
    ev.constraints.minWidth = std::max(ev.constraints.minWidth, bound.x);
    ev.constraints.minHeight = std::max(ev.constraints.minHeight, bound.y);
}

template<>
void handle_event(RenderEvent& ev, entt::entity ent, TextBoxComp& comp, entt::registry& reg) {
    sf::Vector2f pos = Physics::getWorldPos(ent, reg);
    if (auto* boundsComp = reg.try_get<BoundsComp>(ent)) {
        pos += boundsComp->bounds.position;
        pos.y += boundsComp->bounds.size.y;
    }
    pos.y *= -1.f;

    comp.text.setPosition(pos);

    UISystem& sys = reg.ctx().get<UISystem>();
    if (sys.activeTextbox == ent) {
        if (comp.selectionActive && comp.selectionStart != comp.selectionEnd) {
            float startX = comp.text.findCharacterPos(comp.selectionStart - comp.viewPos).x;
            float endX = comp.text.findCharacterPos(comp.selectionEnd - comp.viewPos).x;
            float startY = pos.y;

            float selX = std::min(startX, endX);
            float selW = std::abs(endX - startX);
            sf::RectangleShape sel(sf::Vector2f(selW, comp.text.getCharacterSize() + 4.f));
            sel.setPosition(sf::Vector2f(selX, startY));
            sel.setFillColor(sf::Color(162, 162, 255, 128));
            ev.window.draw(sel);
        }
    }

    std::string wasContent = comp.text.getString();
    std::string newContent = wasContent;
    for (size_t i = 0; i < wasContent.size(); ++i) {
        if (newContent[i] == '\n') newContent[i] = ' ';
    }
    comp.text.setString(newContent);
    ev.window.draw(comp.text);
    comp.text.setString(wasContent);

    if (sys.activeTextbox == ent) {
        uint64_t ticks = reg.ctx().get<uint64_t>();
        if ((ticks / 30) % 2 == 0) {
            sf::RectangleShape cursor(sf::Vector2f(2.f, comp.text.getCharacterSize()));
            cursor.setFillColor(sf::Color::White);
            float cursorX = comp.text.findCharacterPos(comp.cursorPos - comp.viewPos).x;
            float cursorY = pos.y;
            cursor.setPosition(sf::Vector2f(cursorX, cursorY));
            ev.window.draw(cursor);
        }
    }
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
    if (UIRectComp* rect = reg.try_get<UIRectComp>(ent)) {
        rect->fillColor = comp.isToggled ? comp.onColor : comp.offColor;
    }
    if (comp.cb) comp.cb(comp.isToggled, ent, reg);

    if (comp.isToggled) {
        for (entt::entity exEnt : comp.exclusiveGroup) {
            if (reg.valid(exEnt) && exEnt != ent) {
                if (ToggleButtonComp* exComp = reg.try_get<ToggleButtonComp>(exEnt)) {
                    if (exComp->isToggled) {
                        exComp->isToggled = false;
                        if (UIRectComp* exRect = reg.try_get<UIRectComp>(exEnt)) {
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

template<>
void handle_event(ClickEvent& ev, entt::entity ent, DropdownTriggerComp& comp, entt::registry& reg) {
    if (!ev.pressed) return;
    if (!reg.valid(comp.spawnedList)) {
        auto [world, pos] = Physics::getWorldAndPos(ent, reg);
        comp.spawnedList = comp.buildList(reg, world, ent);
        UI::rebuild(comp.spawnedList, reg);
        PositionComp& posc = reg.get<PositionComp>(comp.spawnedList);
        BoundsComp& bounds = reg.get<BoundsComp>(comp.spawnedList);
        posc.position = pos - sf::Vector2f{0.f, bounds.bounds.size.y};
    } else {
        queue_delete(comp.spawnedList, reg);
        comp.spawnedList = entt::null;
    }
    ev.handled = true;
}
