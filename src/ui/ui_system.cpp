#include "core/entity.hpp"
#include "graphics/components.hpp"
#include "input/components.hpp"
#include "input/system.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/builder.hpp"
#include "ui/system.hpp"
#include "ui/components.hpp"
#include <SFML/Window/Clipboard.hpp>

void UISystem::init(entt::registry& reg) {
    subscribe_global_event<UpdateEvent, &UISystem::update>(reg, this);
    subscribe_global_event<ScreenResizeEvent, &UISystem::onScreenResize>(reg, this);
    subscribe_global_event<GlobalMouseMoveEvent, &UISystem::onGlobalMouseMove>(reg, this);
    subscribe_global_event<KeyPressEvent, &UISystem::onGlobalKeyPress>(reg, this);
    subscribe_global_event<GlobalTextEnteredEvent, &UISystem::onGlobalTextEntered>(reg, this);

    // Load fonts
    const std::filesystem::path fonts_path("resources/fonts");
    for (const auto& font_file : std::filesystem::directory_iterator(fonts_path)) {
        if (font_file.is_regular_file()) {
            auto name = font_file.path().stem().string();
            sf::Font& file_font = font_map[name];
            if (!file_font.openFromFile(font_file))
                font_map.erase(name);
            else
                font_name_map[&file_font] = name;
        }
    }
}

void UISystem::update(const UpdateEvent& ev) {
    uint64_t currentTick = ev.registry->ctx().get<uint64_t>();
    auto view = ev.registry->view<TooltipProviderComp, HoverListenerComp>();
    for (auto [entity, tooltip, hover] : view.each()) {
        if (hover.lastHoverTick + 1 < currentTick) {
            // Not hovered
            if (ev.registry->valid(tooltip.spawnedTooltip)) {
                queue_delete(tooltip.spawnedTooltip, *ev.registry);
                tooltip.spawnedTooltip = entt::null;
            }
        } else {
            // Hovered
            if (hover.hoverTime >= tooltip.threshold && !ev.registry->valid(tooltip.spawnedTooltip)) {
                entt::entity world = Physics::getWorld(entity, *ev.registry);
                if (!ev.registry->valid(world)) continue;

                sf::FloatRect ttBounds{hover.lastWorldCoords + sf::Vector2f(12.f, -212.f), {100.f, 200.f}};

                UIBuilder tooltipEnt = UIBuilder(*ev.registry, world, "Tooltip")
                    .posAbsolute(ttBounds)
                    .zIndex(z_ui + 1024)
                    .rect(sf::Color(20, 20, 20, 230), sf::Color(200, 200, 200), 1.f)
                    .childText(tooltip.text, "hack", 12, sf::Color::White, true);

                auto& bounds = ev.registry->get<BoundsComp>(tooltipEnt);
                bounds.resize({{0.f, 0.f}, {100.f, 200.f}}, tooltipEnt, *ev.registry);

                tooltip.spawnedTooltip = tooltipEnt;
            }
        }
    }

    // Dropdown autoclose and hover-trigger logic
    auto ddView = ev.registry->view<DropdownTriggerComp>();
    for (auto [entity, dd] : ddView.each()) {
        if (!ev.registry->valid(dd.spawnedList) && dd.openOnHover) {
            // Check hover open
            if (auto* h = ev.registry->try_get<HoverListenerComp>(entity)) {
                if (h->lastHoverTick + 1 >= currentTick && h->hoverTime >= dd.hoverThreshold) {
                    entt::entity world = Physics::getWorld(entity, *ev.registry);
                    dd.spawnedList = dd.buildList(*ev.registry, world, entity);
                }
            }
        }
    }

    auto colView = ev.registry->view<CloseOnLeaveComp, HoverListenerComp>();
    for (auto [entity, col, hover] : colView.each()) {
        bool overTrigger = false;
        if (ev.registry->valid(col.triggerEntity)) {
            if (auto* trigHover = ev.registry->try_get<HoverListenerComp>(col.triggerEntity)) {
                overTrigger = (trigHover->lastHoverTick + 1 >= currentTick);
            }
        }
        bool overSelf = (hover.lastHoverTick + 1 >= currentTick);

        if (!overTrigger && !overSelf) {
            col.hoverTime += ev.dt;
            if (col.hoverTime >= col.threshold) {
                queue_delete(entity, *ev.registry);
            }
        } else {
            col.hoverTime = 0.f;
        }
    }
}

void UISystem::onScreenResize(const ScreenResizeEvent& ev) {
    entt::registry& reg = *ev.registry;

    auto view = reg.view<UIScreenComp>();
    for (auto [entity, screen] : view.each()) {
        if (!reg.valid(screen.camera)) continue;

        CameraComp* cam = reg.try_get<CameraComp>(screen.camera);
        BoundsComp* bounds = reg.try_get<BoundsComp>(entity);
        if (!cam || !bounds) continue;

        float scale = (cam->scale > 0.f) ? cam->scale : 1.f;
        sf::Vector2f viewSize{
            (float)ev.width / scale,
            (float)ev.height / scale
        };

        // Update root bounds (fires BoundsResizeEvent)
        bounds->resize(sf::FloatRect{{0.f, 0.f}, viewSize}, entity, reg);

        // Reposition the camera to the centre of the UI area
        if (PositionComp* camPos = reg.try_get<PositionComp>(screen.camera)) {
            camPos->position = viewSize * 0.5f;
        }
    }
}

void UISystem::onGlobalMouseMove(const GlobalMouseMoveEvent& ev) {
    entt::registry& reg = *ev.registry;

    auto view = reg.view<DraggableComp, PositionComp>();
    for (auto [entity, drag, pos] : view.each()) {
        if (!drag.being_dragged)
            continue;

        if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            drag.being_dragged = false;
            continue;
        }

        auto [world, selfPos] = Physics::getWorldAndPos(entity, reg);
        auto newPosMaybe = Input::get_world_mouse_pos(ev.pixelCoords, world, reg);
        if (!newPosMaybe)
            continue;
        sf::Vector2f newPos = *newPosMaybe;
        sf::Vector2f newRelative = newPos - selfPos;
        sf::Vector2f deltaRelative = newRelative - drag.anchorCoords;
        pos.position += deltaRelative;
    }

    if (reg.valid(activeTextbox)) {
        if (auto* tb = reg.try_get<TextBoxComp>(activeTextbox)) {
            if (tb->clickDragged) {
                if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    tb->clickDragged = false;
                    if (tb->selectionStart == tb->selectionEnd) {
                        tb->selectionActive = false;
                    }
                } else {
                    auto [world, selfPos] = Physics::getWorldAndPos(activeTextbox, reg);
                    auto mousePosMaybe = Input::get_world_mouse_pos(ev.pixelCoords, world, reg);
                    if (mousePosMaybe) {
                        sf::Vector2f mousePos = *mousePosMaybe;
                        tb->selectionEnd = std::min(tb->viewPos + tb->getMouseCursorPos(mousePos.x), tb->content.size());
                        tb->cursorPos = tb->selectionEnd;
                        if (tb->selectionStart != tb->selectionEnd) tb->selectionActive = true;
                    }
                }
            }
        }
    }
}

void UISystem::onGlobalTextEntered(const GlobalTextEnteredEvent& ev) {
    if (!ev.registry->valid(activeTextbox)) return;
    if (auto* tb = ev.registry->try_get<TextBoxComp>(activeTextbox)) {
        if (ev.unicode > 31 && ev.unicode < 128 && (tb->selectionActive || tb->content.size() < tb->maxChars)) {
            float width = ev.registry->get<BoundsComp>(activeTextbox).bounds.size.x;
            if (tb->selectionActive) tb->eraseSelection(width);
            tb->content.insert(tb->cursorPos, 1, (char)ev.unicode);
            tb->cursorPos++;
            tb->stringChanged(width);
            *ev.handled = true;
        }
    }
}

void UISystem::onGlobalKeyPress(const KeyPressEvent& ev) {
    if (ev.registry->valid(activeTextbox)) {
        if (auto* tb = ev.registry->try_get<TextBoxComp>(activeTextbox)) {
            bool handled = true;
            float width = ev.registry->get<BoundsComp>(activeTextbox).bounds.size.x;

            auto& fullString = tb->content;
            auto& cursorPos = tb->cursorPos;
            auto& selectionActive = tb->selectionActive;

            switch (ev.key) {
                case sf::Keyboard::Key::Enter: {
                    if (tb->onEnter) tb->onEnter(tb->content, activeTextbox, *ev.registry);
                    activeTextbox = entt::null;
                    break;
                }
                case sf::Keyboard::Key::Escape: {
                    activeTextbox = entt::null;
                    break;
                }
                case sf::Keyboard::Key::Backspace: {
                    if (selectionActive) {
                        tb->eraseSelection(width);
                    } else if (fullString.size() > 0 && cursorPos > 0) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                            size_t from = cursorPos;
                            cursorPos--;
                            while (cursorPos > 0 && !(fullString[cursorPos - 1] == ' ' && fullString[cursorPos] != ' ')) {
                                cursorPos--;
                            }
                            fullString.erase(cursorPos, from - cursorPos);
                        } else {
                            fullString.erase(cursorPos - 1, 1);
                            cursorPos = cursorPos - 1;
                        }
                        tb->stringChanged(width);
                    }
                    break;
                }
                case sf::Keyboard::Key::Delete: {
                    if (selectionActive) {
                        tb->eraseSelection(width);
                    } else if (fullString.size() > 0 && cursorPos < fullString.size()) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                            size_t from = cursorPos;
                            cursorPos++;
                            while (cursorPos < fullString.size() && !(fullString[cursorPos - 1] != ' ' && fullString[cursorPos] == ' ')) {
                                cursorPos++;
                            }
                            fullString.erase(from, cursorPos - from);
                        } else {
                            fullString.erase(cursorPos, 1);
                        }
                        tb->stringChanged(width);
                    }
                    break;
                }
                case sf::Keyboard::Key::Left: {
                    if (fullString.size() == 0 || cursorPos == 0) break;
                    bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
                    size_t oldPos = cursorPos;
                    if (selectionActive && !shiftHeld) {
                        cursorPos = std::min(tb->selectionStart, tb->selectionEnd);
                        selectionActive = false;
                    } else {
                        cursorPos--;
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                        while (cursorPos > 0 && !(fullString[cursorPos - 1] == ' ' && fullString[cursorPos] != ' ')) {
                            cursorPos--;
                        }
                    }
                    if (shiftHeld) {
                        if (!selectionActive) {
                            selectionActive = true;
                            tb->selectionStart = oldPos;
                        }
                        tb->selectionEnd = cursorPos;
                    }
                    tb->stringChanged(width);
                    break;
                }
                case sf::Keyboard::Key::Right: {
                    if (cursorPos == fullString.size()) break;
                    bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
                    size_t oldPos = cursorPos;
                    if (selectionActive && !shiftHeld) {
                        cursorPos = std::max(tb->selectionStart, tb->selectionEnd);
                        selectionActive = false;
                    } else {
                        cursorPos = std::min(cursorPos + 1, fullString.size());
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                        while (cursorPos < fullString.size() && !(fullString[cursorPos - 1] != ' ' && fullString[cursorPos] == ' ')) {
                            cursorPos++;
                        }
                    }
                    if (shiftHeld) {
                        if (!selectionActive) {
                            selectionActive = true;
                            tb->selectionStart = oldPos;
                        }
                        tb->selectionEnd = cursorPos;
                    }
                    tb->stringChanged(width);
                    break;
                }
                case sf::Keyboard::Key::A: {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                        selectionActive = true;
                        tb->selectionStart = 0;
                        tb->selectionEnd = fullString.size();
                    } else { handled = false; }
                    break;
                }
                case sf::Keyboard::Key::C: {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && selectionActive) {
                        sf::Clipboard::setString(fullString.substr(std::min(tb->selectionStart, tb->selectionEnd), std::max(tb->selectionStart, tb->selectionEnd) - std::min(tb->selectionStart, tb->selectionEnd)));
                    } else { handled = false; }
                    break;
                }
                case sf::Keyboard::Key::V: {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                        std::string pasted = sf::Clipboard::getString().toAnsiString();
                        pasted.erase(std::remove_if(pasted.begin(), pasted.end(), [](char c) { return c < 32 || c > 126; }), pasted.end());
                        size_t selection = selectionActive ? std::max(tb->selectionStart, tb->selectionEnd) - std::min(tb->selectionStart, tb->selectionEnd) : 0;
                        if (fullString.size() + pasted.size() - selection > tb->maxChars) {
                            pasted = pasted.substr(0, tb->maxChars - fullString.size() + selection);
                        }
                        if (selectionActive) {
                            fullString.replace(std::min(tb->selectionStart, tb->selectionEnd), selection, pasted);
                            tb->selectionStart = std::min(tb->selectionStart, tb->selectionEnd);
                            tb->selectionEnd = tb->selectionStart + pasted.size();
                        } else {
                            fullString.insert(cursorPos, pasted);
                        }
                        cursorPos += pasted.size() - selection;
                        tb->stringChanged(width);
                    } else { handled = false; }
                    break;
                }
                default:
                    if (ev.key < sf::Keyboard::Key::F1 || ev.key > sf::Keyboard::Key::F15) {
                        handled = true; // swallow ordinary character keys so we don't trigger game actions while typing
                    } else {
                        handled = false;
                    }
                    break;
            }
            if (handled) {
                *ev.handled = true;
                return;
            }
        }
    }

    auto triggerView = ev.registry->view<ButtonToggledUIComp, UIComp>();
    for (auto [entity, toggled, ui] : triggerView.each()) {
        if (ev.key == toggled.triggerKey)
            ui.set_hidden(!ui.selfHidden, *ev.registry, entity);
    }
}

namespace UI {
    static void clear_caches(entt::entity node, entt::registry& reg) {
        UIComp& ui = reg.get<UIComp>(node);
        ui._cachedConstraints.reset();
        for (auto child : ui.children) {
            if (reg.valid(child)) {
                clear_caches(child, reg);
            }
        }
    }

    void rebuild(entt::entity node, entt::registry& reg) {
        entt::entity root = node;
        while (reg.valid(root)) {
            PositionComp& pos = reg.get<PositionComp>(root);
            entt::entity parent = pos.parent();
            if (!reg.valid(parent) || parent == root || !reg.try_get<UIComp>(parent)) {
                break;
            }
            root = parent;
        }

        clear_caches(root, reg);

        BoundsComp& bounds = reg.get<BoundsComp>(root);
        bounds.resize(bounds.bounds, root, reg);
    }
}
