#pragma once
#include <SFML/Graphics/Text.hpp>
#include <functional>

#include "graphics/events.hpp"
#include "input/events.hpp"
#include "serialization/serialization.hpp"

// run code on click
struct ButtonComp {
    std::function<void(ClickEvent& ev)> exec;

    void OnClick(ClickEvent&);
};

struct TextComp {
    void OnRender(RenderEvent&);

    sf::Text text;

    REGISTER_SERIALIZABLE(TextComp, Text)
};
