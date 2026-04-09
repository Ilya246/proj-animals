#pragma once
#include <functional>
#include "input/events.hpp"

// run code on click
struct ButtonComp {
    std::function<void(ClickEvent& ev)> exec;

    void OnClick(ClickEvent&);
};
