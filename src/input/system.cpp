#include <entt/entt.hpp>

#include "core/events.hpp"
#include "graphics/components.hpp"
#include "input/components.hpp"
#include "input/events.hpp"
#include "physics/components.hpp"
#include "physics/events.hpp"
#include "physics/system.hpp"
#include "utility/utility.hpp"
#include "input/system.hpp"
#include "world/components.hpp"

void InputSystem::init(entt::registry& reg) {
    subscribe_global_event<UpdateEvent, &InputSystem::update>(reg, this);
    subscribe_global_event<GlobalClickEvent, &InputSystem::receiveClick>(reg, this);
    subscribe_local_event<InputMovementComp, GetDragEvent, &InputMovementComp::OnGetDrag>(reg);
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
    }
}

namespace Input {
    sf::Vector2f get_cam_mouse_pos(sf::Vector2i screenPos, entt::entity camera, entt::registry& reg) {
        CameraComp& cam = reg.get<CameraComp>(camera);
        sf::Vector2f camOrigin = cam.view.getCenter() - cam.view.getSize() * 0.5f;
        sf::Vector2f windowSize = (sf::Vector2f)reg.ctx().get<sf::RenderWindow&>().getSize();
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
    std::map<entt::entity, sf::Vector2f> clickMap;
    auto camView = ev.registry->view<CameraComp>();
    for (auto [entity, cam] : camView.each()) {
        auto worldEnt = Physics::getWorld(entity, *ev.registry);
        sf::Vector2f coords = Input::get_cam_mouse_pos(ev.pixelCoords, entity, *ev.registry);
        clickMap[worldEnt] = coords;
    }

    auto clickableView = ev.registry->view<ClickListenerComp, PositionComp>();
    for (auto [entity, clickable, position] : clickableView.each()) {
        auto [worldEnt, worldPos] = Physics::getWorldAndPos(entity, *ev.registry);

        // don't count the click if we're not being drawn
        bool nonRenderable = false;
        ShouldRenderEvent shouldEv(entity, ev.registry, &nonRenderable);
        raise_local_event(*ev.registry, entity, shouldEv);
        if (nonRenderable || !clickMap.contains(worldEnt))
            continue;

        sf::Vector2f clickPos = clickMap.at(worldEnt);

        sf::FloatRect bounds = get_optional_bounds(entity, clickable, *ev.registry);
        bounds.position += worldPos;
        if (bounds.contains(clickPos))
            raise_local_event(*ev.registry, entity, ClickEvent{ev.pixelCoords, clickPos - worldPos, clickPos, ev.button, entity, ev.pressed, ev.registry});
    }
}

// no drag
void InputMovementComp::OnGetDrag(GetDragEvent& ev) {
    if (lastInput != zeroVec)
        *ev.drag = 0.f;
}
