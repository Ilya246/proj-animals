#include "core/events.hpp"
#include "graphics/events.hpp"
#include "input/events.hpp"
#include "physics/system.hpp"
#include "ui/components.hpp"
#include "ui/system.hpp"

void UISystem::init(entt::registry& reg) {
    subscribe_local_event<ButtonComp, ClickEvent, &ButtonComp::OnClick>(reg);
    subscribe_local_event<TextComp, RenderEvent, &TextComp::OnRender>(reg);

    // load fonts
    const std::filesystem::path textures_path("resources/fonts");
    for (const auto& font_file : std::filesystem::directory_iterator(textures_path)) {
        if (font_file.is_regular_file()) {
            auto name = font_file.path().stem();
            sf::Font& file_font = font_map[name];
            if (!file_font.openFromFile(font_file))
                font_map.erase(name);
            else
                font_name_map[&file_font] = name;
        }
    }
}

void ButtonComp::OnClick(ClickEvent& ev) {
    exec(ev);
}

void TextComp::OnRender(RenderEvent& ev) {
    sf::Vector2f pos = Physics::worldPos(ev.ent, *ev.reg);
    pos.y *= -1.f;
    text.setPosition(pos);
    ev.window->draw(text);
}