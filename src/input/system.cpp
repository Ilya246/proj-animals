#include <SFML/System/Vector2.hpp>
#include <entt/entt.hpp>

#include "core/events.hpp"
#include "graphics/components.hpp"
#include "input/components.hpp"
#include "input/events.hpp"
#include "physics/components.hpp"
#include "physics/events.hpp"
#include "input/system.hpp"
#include "world/components.hpp"

void InputSystem::init(entt::registry& reg) {
    subscribe_global_event<UpdateEvent, &InputSystem::update>(reg, this);
    subscribe_global_event<GlobalClickEvent, &InputSystem::receiveClick>(reg, this);
    subscribe_global_event<GlobalScrollEvent, &InputSystem::receiveScroll>(reg, this);
}

void InputSystem::update(const UpdateEvent& ev) {
    auto view = ev.registry->view<InputMovementComp, PhysicsComp>();
    for (auto [entity, mover, vel] : view.each()) {
        // Player input (arrow keys)
        sf::Vector2i input(0, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            input.x = -1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            input.x = 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            input.y = 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            input.y = -1;

        mover.lastInput = (sf::Vector2f)input;
        if (input.x == 0 && input.y == 0)
            continue;

        sf::Vector2f targetVel = ((sf::Vector2f)input).normalized() * mover.speed;

        // Update player velocity component
        PhysicsComp& playerVelComp = ev.registry->get<PhysicsComp>(entity);
        sf::Vector2f deltaVel = targetVel - playerVelComp.velocity;
        if (deltaVel != zeroVec)
            playerVelComp.velocity += deltaVel.normalized() * mover.accel * ev.dt;

        InputMovedEvent iEv((sf::Vector2f)input);
        raise_local_event(*ev.registry, entity, iEv);
    }

    sf::RenderWindow& window = ev.registry->ctx().get<sf::RenderWindow>();
    if (window.hasFocus()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        bool handled = false;
        GlobalHoverEvent hEv{mousePos, ev.registry, &handled};
        uint64_t currentTick = ev.registry->ctx().get<uint64_t>();

        Input::handle_mouse_event<HoverEvent, HoverListenerComp, GlobalHoverEvent, true>(hEv,
            [&](sf::Vector2f relPos, sf::Vector2f globalPos, entt::entity ent, bool* handled) {
                raise_local_event(*ev.registry, ent, HoverEvent{mousePos, relPos, globalPos, ev.dt, currentTick, *handled});
            });
    }
}

namespace Input {
    sf::Vector2f get_cam_mouse_pos(sf::Vector2i screenPos, entt::entity camera, entt::registry& reg) {
        CameraComp& cam = reg.get<CameraComp>(camera);
        sf::Vector2f camOrigin = cam.view.getCenter() - cam.view.getSize() * 0.5f;
        sf::Vector2f windowSize = (sf::Vector2f)reg.ctx().get<sf::RenderWindow>().getSize();
        sf::Vector2f viewSize = cam.view.getSize();
        float scaleX = viewSize.x / windowSize.x;
        float scaleY = viewSize.y / windowSize.y;
        sf::Vector2f relCoords = sf::Vector2f{screenPos.x * scaleX, screenPos.y * scaleY};
        sf::Vector2f coords = camOrigin + relCoords;
        coords.y *= -1.f; // convert to world coordinates
        return coords;
    }

    std::optional<sf::Vector2f> get_world_mouse_pos(sf::Vector2i screenPos, entt::entity world, entt::registry& reg) {
        auto* worldC = reg.try_get<WorldComp>(world);
        if (!worldC) return {};
        return get_cam_mouse_pos(screenPos, worldC->lastCamera, reg);
    }
}

void InputSystem::receiveClick(const GlobalClickEvent& ev) {
    Input::handle_mouse_event<ClickEvent, ClickListenerComp, GlobalClickEvent, true>(ev,
        [&ev](sf::Vector2f relPos, sf::Vector2f globalPos, entt::entity ent, bool* handled) {
            raise_local_event(*ev.registry, ent, ClickEvent{ev.pixelCoords, relPos, globalPos, ev.button, ev.pressed, *handled});
        });
}

void InputSystem::receiveScroll(const GlobalScrollEvent& ev) {
    Input::handle_mouse_event<ScrollEvent, ScrollListenerComp, GlobalScrollEvent, true>(ev,
        [&ev](sf::Vector2f relPos, sf::Vector2f globalPos, entt::entity ent, bool* handled) {
            return raise_local_event(*ev.registry, ent, ScrollEvent{ev.pixelCoords, relPos, globalPos, ev.delta, *handled});
        });
}

// no drag
template<>
void handle_event(GetDragEvent& ev, entt::entity, InputMovementComp& comp, entt::registry&) {
    if (comp.lastInput != zeroVec)
        ev.drag = 0.f;
}

template<>
void handle_event(HoverEvent& ev, entt::entity, HoverListenerComp& comp, entt::registry&) {
    if (ev.tick > comp.lastHoverTick + 1) {
        comp.hoverTime = 0.f;
    }
    comp.hoverTime += ev.dt;
    comp.lastHoverTick = ev.tick;
    comp.lastWorldCoords = ev.worldCoords;
    ev.handled = true;
}
