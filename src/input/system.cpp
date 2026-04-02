#include <entt/entt.hpp>

#include "core/events.hpp"
#include "input/components.hpp"
#include "physics/components.hpp"
#include "physics/events.hpp"
#include "serialization/serialization.hpp"
#include "utility/utility.hpp"
#include "input/system.hpp"

void InputSystem::init(entt::registry& reg) {
    subscribeGlobalEvent<UpdateEvent, &InputSystem::update>(reg, this);
    subscribeLocalEvent<InputMovementComp, GetDragEvent, &InputMovementComp::OnGetDrag>(reg);

    ComponentSerializer::register_component<InputMovementComp>("InputMovement");
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

// no drag
void InputMovementComp::OnGetDrag(GetDragEvent& ev) {
    if (lastInput != zeroVec)
        *ev.drag = 0.f;
}