#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

struct SpriteComp {
    sf::Sprite sprite;
};

// Renders entities with the same world parent in bounds. 
struct CameraComp {
    float scale;
    sf::View view;
};