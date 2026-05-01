#include "utility/utility.hpp"

#ifdef APROJ_DEBUG
void exec_debug(std::function<void()>&& fun) {
    fun();
}
#else
void exec_debug(std::function<void()>&&) {}
#endif

DynamicBounds::DynamicBounds(float pX, float pY, float sX, float sY, std::array<bool, 4> fractionMode) {
    bounds = {{pX, pY}, {sX, sY}};
    isFraction = fractionMode;
}

sf::FloatRect apply_dynamic_bounds(sf::FloatRect onto, DynamicBounds bounds) {
    sf::Vector2f basePos = onto.position;
    sf::Vector2f baseSize = onto.size;
    sf::Vector2f pos = bounds.bounds.position;
    sf::Vector2f size = bounds.bounds.size;
    sf::Vector2f setSize = {bounds.isFraction[0] ? size.x * baseSize.x : size.x, bounds.isFraction[1] ? size.y * baseSize.y : size.y};
    sf::Vector2f setPos = {bounds.isFraction[2] ? pos.x * basePos.x : pos.x, bounds.isFraction[3] ? pos.y * basePos.y : pos.y};
    return {setPos, setSize};
}