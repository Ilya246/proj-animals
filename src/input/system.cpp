#include <entt/entt.hpp>

#include "core/events.hpp"
#include "input/components.hpp"
#include "physics/components.hpp"
#include "utility/utility.hpp"
#include "input/system.hpp"

REGISTER_SYSTEM(InputSystem);

void InputSystem::init(entt::registry& reg) {
    entt::dispatcher& dispatch = ev_dispatcher(reg);

    dispatch.sink<UpdateEvent>().connect<&InputSystem::update>(this);
}

void InputSystem::update(const UpdateEvent& ev) {
    auto view = ev.registry->view<PhysicsComp, InputMovementComp>();
    for (auto [entity, vel, mover] : view.each()) {
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

        if (input.x == 0 && input.y == 0)
            continue;

        sf::Vector2f targetVel = ((sf::Vector2f)input).normalized() * mover.speed;

        // Update player velocity component
        PhysicsComp& playerVelComp = ev.registry->get<PhysicsComp>(entity);
        sf::Vector2f deltaVel = targetVel - playerVelComp.velocity;
        // Counteract drag
        if (playerVelComp.velocity != zeroVec)
            playerVelComp.velocity += playerVelComp.velocity.normalized() * vel.drag * ev.dt;
        if (deltaVel != zeroVec)
            playerVelComp.velocity += deltaVel.normalized() * mover.accel * ev.dt;
    }
}