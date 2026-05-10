#pragma once
#include <SFML/Graphics.hpp>

struct RenderEvent {
    sf::RenderWindow& window;
};

struct ShouldRenderEvent {
    bool& cancelled;
    // stencil bounds, in world coordinates
    std::optional<sf::FloatRect>& stencil;
};
