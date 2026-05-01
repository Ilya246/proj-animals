#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <functional>

// Executes given function if APROJ_DEBUG is defined
// else, does nothing
void exec_debug(std::function<void()>&& fun);
