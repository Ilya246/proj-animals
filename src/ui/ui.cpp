#include <SFML/Graphics/Rect.hpp>

#include "core/events.hpp"
#include "graphics/components.hpp"
#include "graphics/events.hpp"
#include "input/events.hpp"
#include "physics/components.hpp"
#include "physics/events.hpp"
#include "physics/system.hpp"
#include "ui/components.hpp"
#include "ui/events.hpp"
#include "ui/system.hpp"
#include "ui/font.hpp"
#include "utility/utility.hpp"

void UISystem::init(entt::registry& reg) {
    subscribe_global_event<ScreenResizeEvent, &UISystem::onScreenResize>(reg, this);

    subscribe_local_event<UIHiddenComp, ShouldRenderEvent, &UIHiddenComp::OnTryDraw>(reg);
    subscribe_local_event<UIFullAllocatorComp, BoundsResizeEvent, &UIFullAllocatorComp::OnResize>(reg);
    subscribe_local_event<UILayoutComp, BoundsResizeEvent, &UILayoutComp::OnResize>(reg);
    subscribe_local_event<UIFillComp, UISizeAllocatedEvent, &UIFillComp::OnAllocate>(reg);
    subscribe_local_event<UIAnchorComp, UISizeAllocatedEvent, &UIAnchorComp::OnAllocate>(reg);
    subscribe_local_event<UIRectComp, RenderEvent, &UIRectComp::OnRender>(reg);
    subscribe_local_event<ButtonComp, ClickEvent, &ButtonComp::OnClick>(reg);
    subscribe_local_event<TextComp, RenderEvent, &TextComp::OnRender>(reg);
    subscribe_local_event<TextComp, BoundsResizeEvent, &TextComp::OnResize>(reg);

    // load fonts
    const std::filesystem::path fonts_path("resources/fonts");
    for (const auto& font_file : std::filesystem::directory_iterator(fonts_path)) {
        if (font_file.is_regular_file()) {
            auto name = font_file.path().stem().string();
            sf::Font& file_font = font_map[name];
            if (!file_font.openFromFile(font_file))
                font_map.erase(name);
            else
                font_name_map[&file_font] = name;
        }
    }
}

void UISystem::onScreenResize(const ScreenResizeEvent& ev) {
    entt::registry& reg = *ev.registry;

    auto view = reg.view<UIScreenComp>();
    for (auto [entity, screen] : view.each()) {
        if (!reg.valid(screen.camera)) continue;

        CameraComp* cam = reg.try_get<CameraComp>(screen.camera);
        BoundsComp* bounds = reg.try_get<BoundsComp>(entity);
        if (!cam || !bounds) continue;

        float scale = (cam->scale > 0.f) ? cam->scale : 1.f;
        sf::Vector2f viewSize{
            static_cast<float>(ev.width) / scale,
            static_cast<float>(ev.height) / scale
        };

        // Update root bounds (fires BoundsResizeEvent → layout / text reflow)
        bounds->resize(sf::FloatRect{{0.f, 0.f}, viewSize}, entity, reg);

        // Reposition the camera to the centre of the UI area
        if (PositionComp* camPos = reg.try_get<PositionComp>(screen.camera)) {
            camPos->position = viewSize * 0.5f;
        }
    }
}

///
/// Utility
///

bool any_parent_hidden(entt::entity ent, entt::registry& reg) {
    if (auto* hidden = reg.try_get<UIHiddenComp>(ent))
        if (hidden->hidden)
            return true;

    PositionComp& pos = reg.get<PositionComp>(ent);
    if (pos.parent == ent)
        return false;

    return any_parent_hidden(pos.parent, reg);
}

void UIHiddenComp::OnTryDraw(ShouldRenderEvent& ev) {
    *ev.cancelled = any_parent_hidden(ev.ent, *ev.reg);
}

///
/// Allocators
///

void UIFullAllocatorComp::OnResize(BoundsResizeEvent& ev) {
    for (entt::entity child : children) {
        ev.registry->get<PositionComp>(child).position = zeroVec;
        raise_local_event(*ev.registry, child, UISizeAllocatedEvent{ev.newBounds, ev.registry, child});
    }
}

void UILayoutComp::OnResize(BoundsResizeEvent& ev) {
    entt::registry& reg = *ev.registry;
    entt::entity entity = ev.entity;

    const sf::FloatRect area = ev.newBounds;
    if (area.size.x <= 0.f || area.size.y <= 0.f) return;

    // Account for UIRectComp border insets
    float borderTop = 0.f, borderBottom = 0.f;
    float borderLeft = 0.f, borderRight = 0.f;
    if (auto* rect = reg.try_get<UIRectComp>(entity)) {
        float bt = rect->borderThickness;
        borderTop = borderBottom = borderLeft = borderRight = bt;
    }

    // Content rectangle (all in local pos-coordinates, Y-up)
    const float contentLeft   = area.position.x + borderLeft + padding;
    const float contentBottom = area.position.y + borderBottom + padding;
    const float contentRight  = area.position.x + area.size.x - borderRight - padding;
    const float contentTop    = area.position.y + area.size.y - borderTop - padding;
    const float contentWidth  = contentRight - contentLeft;
    const float contentHeight = contentTop - contentBottom;

    if (contentWidth <= 0.f || contentHeight <= 0.f) return;

    if (children.empty()) return;

    // down-up
    if (mode == UILayoutMode::Vertical) {
        float fillHeight = contentHeight / children.size();
        float totalSpacing = spacing * (children.size() - 1.f);
        float avgSpacing = totalSpacing / children.size();
        float cursorY = contentBottom; // pos-coords: top = high Y

        for (auto child : children) {
            float childWidth = contentWidth;
            float childHeight = fillHeight - avgSpacing;

            sf::Vector2f pos = sf::Vector2f(contentLeft, cursorY);
            reg.get<PositionComp>(child).position = pos;
            sf::FloatRect bounds = sf::FloatRect{zeroVec, {childWidth, childHeight}};
            raise_local_event(reg, child, UISizeAllocatedEvent{bounds, &reg, child});

            cursorY += fillHeight;
        }
    // left-right
    } else {
        float fillWidth = contentWidth / children.size();
        float totalSpacing = spacing * (children.size() - 1.f);
        float avgSpacing = totalSpacing / children.size();
        float cursorX = contentLeft;

        for (auto child : children) {
            float childHeight = contentHeight;
            float childWidth = fillWidth - avgSpacing;

            sf::Vector2f pos = sf::Vector2f(cursorX, contentBottom);
            reg.get<PositionComp>(child).position = pos;
            sf::FloatRect bounds = sf::FloatRect{zeroVec, {childWidth, childHeight}};
            raise_local_event(reg, child, UISizeAllocatedEvent{bounds, &reg, child});

            cursorX += fillWidth;
        }
    }
}

///
/// Allocation handlers
///

void UIFillComp::OnAllocate(UISizeAllocatedEvent& ev) {
    ev.registry->get<BoundsComp>(ev.entity).resize(ev.bounds, ev.entity, *ev.registry);
}

void UIAnchorComp::OnAllocate(UISizeAllocatedEvent& ev) {
    sf::FloatRect alloc = ev.bounds;
    sf::Vector2f scaledPos = {alloc.position.x + alloc.size.x * bounds.position.x, alloc.position.y + alloc.size.y * bounds.position.y};
    sf::Vector2f scaledSize = {alloc.size.x * bounds.size.x, alloc.size.y * bounds.size.y};
    ev.registry->get<BoundsComp>(ev.entity).resize({scaledPos, scaledSize}, ev.entity, *ev.registry);
}

///
/// Elements
///

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
