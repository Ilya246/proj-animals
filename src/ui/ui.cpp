#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>

#include "core/events.hpp"
#include "graphics/components.hpp"
#include "graphics/events.hpp"
#include "input/events.hpp"
#include "input/system.hpp"
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
    subscribe_global_event<GlobalMouseMoveEvent, &UISystem::onGlobalMouseMove>(reg, this);
    subscribe_global_event<KeyPressEvent, &UISystem::onGlobalKeyPress>(reg, this);

    subscribe_local_event<UIComp, ShouldRenderEvent, &UIComp::OnTryDraw>(reg);
    subscribe_local_event<UIComp, UIPropagateEvent, &UIComp::OnPropagate>(reg);
    subscribe_local_event<UIComp, BoundsResizeEvent, &UIComp::OnResize>(reg);

    subscribe_local_event<UIFullAllocatorComp, BoundsResizeEvent, &UIFullAllocatorComp::OnResize>(reg);
    subscribe_local_event<UILayoutComp, BoundsResizeEvent, &UILayoutComp::OnResize>(reg);

    subscribe_local_event<UIFillComp, UISizeAllocatedEvent, &UIFillComp::OnAllocate>(reg);
    subscribe_local_event<UIAnchorComp, UISizeAllocatedEvent, &UIAnchorComp::OnAllocate>(reg);
    subscribe_local_event<UIAbsoluteBoundsComp, UISizeAllocatedEvent, &UIAbsoluteBoundsComp::OnAllocate>(reg);

    subscribe_local_event<UIRectComp, RenderEvent, &UIRectComp::OnRender>(reg);
    subscribe_local_event<UIWindowComp, RenderEvent, &UIWindowComp::OnRender>(reg);

    subscribe_local_event<DraggableComp, ClickEvent, &DraggableComp::OnClick>(reg);
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

void UISystem::onGlobalMouseMove(const GlobalMouseMoveEvent& ev) {
    entt::registry& reg = *ev.registry;

    auto view = reg.view<DraggableComp, PositionComp>();
    for (auto [entity, drag, pos] : view.each()) {
        if (!drag.being_dragged)
            continue;

        if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            drag.being_dragged = false;
            continue;
        }

        auto [world, selfPos] = Physics::getWorldAndPos(entity, reg);
        auto newPosMaybe = Input::get_world_mouse_pos(ev.pixelCoords, world, reg);
        if (!newPosMaybe)
            continue;
        sf::Vector2f newPos = *newPosMaybe;
        sf::Vector2f newRelative = newPos - selfPos;
        sf::Vector2f deltaRelative = newRelative - drag.anchorCoords;
        pos.position += deltaRelative;
    }
}

void UISystem::onGlobalKeyPress(const KeyPressEvent& ev) {
    auto triggerView = ev.registry->view<ButtonToggledUIComp, UIComp>();
    for (auto [entity, toggled, ui] : triggerView.each()) {
        if (ev.key == toggled.triggerKey)
            ui.set_hidden(!ui.self_hidden, *ev.registry, entity);
    }
}


///
/// UI Component Methods
///

void UIComp::OnTryDraw(ShouldRenderEvent& ev) {
    if (self_hidden || parent_hidden) {
        *ev.cancelled = true;
        return;
    }
    if (cached_stencil.has_value()) {
        sf::Vector2f pos = Physics::getWorldPos(ev.ent, *ev.reg);
        *ev.stencil = {cached_stencil->position + pos, cached_stencil->size};
    }
}

void UIComp::OnPropagate(UIPropagateEvent& ev) {
    propagate(ev, ev.entity, *ev.registry);
}

void UIComp::OnResize(BoundsResizeEvent& ev) {
    if (is_stencil) {
        sf::FloatRect new_stencil = ev.newBounds;

        // Intersect with parent stencil if one exists
        if (auto* posComp = ev.registry->try_get<PositionComp>(ev.entity)) {
            if (auto* parentUI = ev.registry->try_get<UIComp>(posComp->parent)) {
                if (parentUI->cached_stencil.has_value()) {
                    auto inters = new_stencil.findIntersection(*parentUI->cached_stencil);
                    new_stencil = inters.has_value() ? *inters : sf::FloatRect{{0.f, 0.f}, {0.f, 0.f}};
                }
            }
        }

        assign_stencil(new_stencil, *ev.registry, ev.entity);
    }
}

void UIComp::set_hidden(bool hide, entt::registry& reg, entt::entity ent) {
    self_hidden = hide;

    UIPropagateEvent ev {
        &reg, ent,
        [](entt::registry& r, entt::entity e) {
            if (auto* ui = r.try_get<UIComp>(e)) {
                if (auto* pos = r.try_get<PositionComp>(e)) {
                    if (auto* parent_ui = r.try_get<UIComp>(pos->parent)) {
                        ui->parent_hidden = parent_ui->self_hidden || parent_ui->parent_hidden;
                    }
                }
            }
        }
    };

    propagate(ev, ent, reg);
}

void UIComp::assign_stencil(std::optional<sf::FloatRect> stencil, entt::registry& reg, entt::entity ent) {
    cached_stencil = stencil;

    UIPropagateEvent ev {
        &reg, ent,
        [stencil](entt::registry& r, entt::entity e) {
            if (auto* ui = r.try_get<UIComp>(e)) {
                if (ui->is_stencil && stencil.has_value()) {
                    // Re-intersect with our own bounds if we are also a stencil provider
                    sf::FloatRect bounds = r.get<BoundsComp>(e).bounds;
                    
                    auto inters = bounds.findIntersection(*stencil);
                    ui->cached_stencil = inters.has_value() ? *inters : sf::FloatRect{{0.f, 0.f}, {0.f, 0.f}};
                } else {
                    ui->cached_stencil = stencil;
                }
            }
        }
    };

    propagate(ev, ent, reg);
}

void UIComp::propagate(UIPropagateEvent& propagate, entt::entity ent, entt::registry& reg) {
    propagate.action(reg, ent);
    for (entt::entity child : children) {
        raise_local_event(reg, child, propagate);
    }
}

///
/// Allocators
///

void UIFullAllocatorComp::OnResize(BoundsResizeEvent& ev) {
    sf::FloatRect newBounds = bounds.has_value() ? apply_dynamic_bounds(ev.newBounds, *bounds) : ev.newBounds;

    const std::vector<entt::entity>& children = ev.registry->get<UIComp>(ev.entity).children;

    for (entt::entity child : children) {
        raise_local_event(*ev.registry, child, UISizeAllocatedEvent{newBounds, ev.registry, child});
    }
}

void UILayoutComp::OnResize(BoundsResizeEvent& ev) {
    entt::registry& reg = *ev.registry;
    entt::entity entity = ev.entity;

    sf::FloatRect newBounds = bounds.has_value() ? apply_dynamic_bounds(ev.newBounds, *bounds) : ev.newBounds;

    if (newBounds.size.x <= 0.f || newBounds.size.y <= 0.f) return;

    // Account for UIRectComp border insets
    float borderTop = 0.f, borderBottom = 0.f;
    float borderLeft = 0.f, borderRight = 0.f;
    if (auto* rect = reg.try_get<UIRectComp>(entity)) {
        float bt = rect->borderThickness;
        borderTop = borderBottom = borderLeft = borderRight = bt;
    }

    // Content rectangle (all in local pos-coordinates, Y-up)
    const float contentLeft   = newBounds.position.x + borderLeft + padding;
    const float contentBottom = newBounds.position.y + borderBottom + padding;
    const float contentRight  = newBounds.position.x + newBounds.size.x - borderRight - padding;
    const float contentTop    = newBounds.position.y + newBounds.size.y - borderTop - padding;
    const float contentWidth  = contentRight - contentLeft;
    const float contentHeight = contentTop - contentBottom;

    if (contentWidth <= 0.f || contentHeight <= 0.f) return;

    const std::vector<entt::entity>& children = reg.get<UIComp>(entity).children;

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
            sf::FloatRect bounds = sf::FloatRect{pos, {childWidth, childHeight}};
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
            sf::FloatRect bounds = sf::FloatRect{pos, {childWidth, childHeight}};
            raise_local_event(reg, child, UISizeAllocatedEvent{bounds, &reg, child});

            cursorX += fillWidth;
        }
    }
}

///
/// Allocation handlers
///

void UIFillComp::OnAllocate(UISizeAllocatedEvent& ev) {
    ev.registry->get<PositionComp>(ev.entity).position = ev.bounds.position;
    ev.registry->get<BoundsComp>(ev.entity).resize({zeroVec, ev.bounds.size}, ev.entity, *ev.registry);
}

void UIAnchorComp::OnAllocate(UISizeAllocatedEvent& ev) {
    sf::FloatRect alloc = ev.bounds;
    sf::FloatRect scaledBounds = apply_dynamic_bounds(alloc, bounds);
    ev.registry->get<PositionComp>(ev.entity).position = scaledBounds.position;
    ev.registry->get<BoundsComp>(ev.entity).resize({zeroVec, scaledBounds.size}, ev.entity, *ev.registry);
}

void UIAbsoluteBoundsComp::OnAllocate(UISizeAllocatedEvent& ev) {
    ev.registry->get<BoundsComp>(ev.entity).resize(bounds, ev.entity, *ev.registry);
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
