#include "core/entity.hpp"
#include "graphics/components.hpp"
#include "input/components.hpp"
#include "input/system.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/builder.hpp"
#include "ui/system.hpp"
#include "ui/components.hpp"

void UISystem::init(entt::registry& reg) {
    subscribe_global_event<UpdateEvent, &UISystem::update>(reg, this);
    subscribe_global_event<ScreenResizeEvent, &UISystem::onScreenResize>(reg, this);
    subscribe_global_event<GlobalMouseMoveEvent, &UISystem::onGlobalMouseMove>(reg, this);
    subscribe_global_event<KeyPressEvent, &UISystem::onGlobalKeyPress>(reg, this);

    // Load fonts
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

void UISystem::update(const UpdateEvent& ev) {
    uint64_t currentTick = ev.registry->ctx().get<uint64_t>();
    auto view = ev.registry->view<TooltipProviderComp, HoverListenerComp>();
    for (auto [entity, tooltip, hover] : view.each()) {
        if (hover.lastHoverTick + 1 < currentTick) {
            // Not hovered
            if (ev.registry->valid(tooltip.spawnedTooltip)) {
                queue_delete(tooltip.spawnedTooltip, *ev.registry);
                tooltip.spawnedTooltip = entt::null;
            }
        } else {
            // Hovered
            if (hover.hoverTime >= tooltip.threshold && !ev.registry->valid(tooltip.spawnedTooltip)) {
                entt::entity world = Physics::getWorld(entity, *ev.registry);
                if (!ev.registry->valid(world)) continue;

                sf::FloatRect ttBounds{hover.lastWorldCoords + sf::Vector2f(12.f, -212.f), {100.f, 200.f}};

                UIBuilder tooltipEnt = UIBuilder(*ev.registry, world, "Tooltip")
                    .posAbsolute(ttBounds)
                    .zIndex(z_ui + 1024)
                    .rect(sf::Color(20, 20, 20, 230), sf::Color(200, 200, 200), 1.f)
                    .childText(tooltip.text, "hack", 12, sf::Color::White, true);

                auto& bounds = ev.registry->get<BoundsComp>(tooltipEnt);
                bounds.resize({{0.f, 0.f}, {100.f, 200.f}}, tooltipEnt, *ev.registry);

                tooltip.spawnedTooltip = tooltipEnt;
            }
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
            (float)ev.width / scale,
            (float)ev.height / scale
        };

        // Update root bounds (fires BoundsResizeEvent)
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
