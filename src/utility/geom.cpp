#include "utility/geom.hpp"

DynamicBounds::DynamicBounds(float pX, float pY, float sX, float sY, std::array<bool, 4> fractionMode) {
    bounds = {{pX, pY}, {sX, sY}};
    isFraction = fractionMode;
}

sf::FloatRect apply_dynamic_bounds(const sf::FloatRect& onto, const DynamicBounds& bounds) {
    sf::Vector2f basePos = onto.position;
    sf::Vector2f baseSize = onto.size;
    sf::Vector2f pos = bounds.bounds.position;
    sf::Vector2f size = bounds.bounds.size;
    sf::Vector2f setSize = {bounds.isFraction[0] ? size.x * baseSize.x : size.x, bounds.isFraction[1] ? size.y * baseSize.y : size.y};
    sf::Vector2f setPos = {bounds.isFraction[2] ? pos.x * basePos.x : pos.x, bounds.isFraction[3] ? pos.y * basePos.y : pos.y};
    return {setPos, setSize};
}

sf::FloatRect rect_union(const sf::FloatRect& a, const sf::FloatRect& b) {
    sf::Vector2f aP = a.position;
    sf::Vector2f aS = a.size;
    sf::Vector2f bP = b.position;
    sf::Vector2f bS = b.size;

    sf::Vector2f aTR = aP + aS;
    sf::Vector2f bTR = bP + bS;

    float pX = std::min(aP.x, bP.x);
    float pY = std::min(aP.y, bP.y);
    float pTRX = std::max(aTR.x, bTR.x);
    float pTRY = std::max(aTR.y, bTR.y);

    return {{pX, pY}, {pTRX - pX, pTRY - pY}};
}
