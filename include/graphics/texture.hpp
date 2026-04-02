#pragma once
#include <SFML/Graphics/Texture.hpp>

#include <unordered_map>

inline std::unordered_map<std::string, sf::Texture> tex_map;
inline std::unordered_map<const sf::Texture*, std::string> tex_name_map;
