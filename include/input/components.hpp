#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <optional>
#include "core/events.hpp"
#include "input/events.hpp"
#include "physics/events.hpp"
#include "serialization/serialization.hpp"

struct InputMovementComp {
    float speed;
    float accel;

    sf::Vector2f lastInput{0.f, 0.f};

    HANDLE_EVENT(InputMovementComp, GetDragEvent)

    REGISTER_SERIALIZABLE(InputMovementComp, InputMovement)
};

struct ClickListenerComp {
    // use RenderableComp bounds if not set
    std::optional<sf::FloatRect> bounds = {};

    REGISTER_SERIALIZABLE(ClickListenerComp, ClickListener)
};

struct ScrollListenerComp {
    std::optional<sf::FloatRect> bounds = {};

    REGISTER_SERIALIZABLE(ScrollListenerComp, ScrollListener)
};

struct HoverListenerComp {
    std::optional<sf::FloatRect> bounds = {};
    uint64_t lastHoverTick = 0;
    float hoverTime = 0.f;
    sf::Vector2f lastWorldCoords = {0.f, 0.f};

    HANDLE_EVENT(HoverListenerComp, HoverEvent)
    REGISTER_SERIALIZABLE(HoverListenerComp, HoverListener)
};
