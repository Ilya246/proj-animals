#include "core/events.hpp"
#include "input/events.hpp"
#include "ui/components.hpp"
#include "ui/system.hpp"

void UISystem::init(entt::registry& reg) {
    subscribe_local_event<ButtonComp, ClickEvent, &ButtonComp::OnClick>(reg);
}

void ButtonComp::OnClick(ClickEvent& ev) {
    exec(ev);
}