#pragma once
#include <unordered_map>
#include <string>
#include <SFML/Graphics.hpp>

inline std::unordered_map<std::string, sf::Font> font_map;
inline std::unordered_map<const sf::Font*, std::string> font_name_map;