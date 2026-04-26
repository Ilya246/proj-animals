#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <format>

template<typename TVec>
struct std::formatter<sf::Vector2<TVec>> : std::formatter<string_view> {
    auto format(const sf::Vector2<TVec>& obj, std::format_context& ctx) const {
        std::string temp = std::format("({}, {})", obj.x, obj.y);
        return std::formatter<string_view>::format(temp, ctx);
    }
};

template<typename TRect>
struct std::formatter<sf::Rect<TRect>> : std::formatter<string_view> {
    auto format(const sf::Rect<TRect>& obj, std::format_context& ctx) const {
        std::string temp = std::format("({}, {})", obj.position, obj.size);
        return std::formatter<string_view>::format(temp, ctx);
    }
};
