#pragma once

#include <SFML/System/Vector2.hpp>
#include <format>

template<>
struct std::formatter<sf::Vector2f> : std::formatter<string_view> {
    auto format(const sf::Vector2f& obj, std::format_context& ctx) const {
        std::string temp = std::format("[{}, {}]", obj.x, obj.y);
        return std::formatter<string_view>::format(temp, ctx);
    }
};
