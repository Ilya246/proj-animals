#pragma once
#include <SFML/System/Vector2.hpp>
#include "core/events.hpp"
#include "core/system.hpp"
#include "input/events.hpp"

struct InputSystem : System<InputSystem> {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return 64; };

        void update(const UpdateEvent&);
        void receiveClick(const GlobalClickEvent&);
        void receiveScroll(const GlobalScrollEvent&);
};

namespace Input {
    sf::Vector2f get_cam_mouse_pos(sf::Vector2i screenPos, entt::entity camera, entt::registry& reg);
    // guesses the camera
    std::optional<sf::Vector2f> get_world_mouse_pos(sf::Vector2i screenPos, entt::entity world, entt::registry& reg);
}
