#pragma once
#include <SFML/System/Vector2.hpp>
#include <functional>

const sf::Vector2f zeroVec{0.f, 0.f};

// Executes given function if APROJ_DEBUG is defined
// else, does nothing
void exec_debug(std::function<void()>&& fun);
