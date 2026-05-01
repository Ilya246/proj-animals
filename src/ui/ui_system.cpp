#include "graphics/components.hpp"
#include "input/system.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/system.hpp"
#include "ui/components.hpp"

void UISystem::init(entt::registry& reg) {
    subscribe_global_event<ScreenResizeEvent, &UISystem::onScreenResize>(reg, this);
    subscribe_global_event<GlobalMouseMoveEvent, &UISystem::onGlobalMouseMove>(reg, this);
    subscribe_global_event<KeyPressEvent, &UISystem::onGlobalKeyPress>(reg, this);

    subscribe_local_event<UIComp, ShouldRenderEvent, &UIComp::OnTryDraw>(reg);
    subscribe_local_event<UIComp, UIPropagateEvent, &UIComp::OnPropagate>(reg);
    subscribe_local_event<UIComp, BoundsResizeEvent, &UIComp::OnResize>(reg);

    subscribe_local_event<UIFullAllocatorComp, BoundsResizeEvent, &UIFullAllocatorComp::OnResize>(reg);
    subscribe_local_event<UILayoutComp, BoundsResizeEvent, &UILayoutComp::OnResize>(reg);
    subscribe_local_event<UITileLayoutComp, BoundsResizeEvent, &UITileLayoutComp::OnResize>(reg);

    subscribe_local_event<UIScrollAreaComp, ScrollEvent, &UIScrollAreaComp::OnScroll>(reg);

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
            ui.set_hidden(!ui.selfHidden, *ev.registry, entity);
    }
}