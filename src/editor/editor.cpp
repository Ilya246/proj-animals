#include <SFML/Graphics/RectangleShape.hpp>
#include "core/events.hpp"
#include "editor/system.hpp"
#include "editor/components.hpp"
#include "graphics/components.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "input/system.hpp"
#include "graphics/texture.hpp"
#include "ui/components.hpp"

entt::entity get_editor(entt::registry& reg) {
    auto edView = reg.view<EditorComp>();
    for (auto [ent, edc] : edView.each()) {
        return ent;
    }
    return entt::null;
}

bool haveEditor(entt::registry& reg) {
    bool has = false;
    auto edView = reg.view<EditorComp, UIComp>();
    for (auto [ent, ed, ui] : edView.each()) {
        has = !ui.selfHidden && !ui._parentHidden;
        break;
    }
    return has;
}

template<>
void handle_event(RenderEvent& ev, entt::entity ent, EditorSelectedComp&, entt::registry& reg) {
    auto* boundsComp = reg.try_get<BoundsComp>(ent);
    if (!boundsComp) return;
    if (!haveEditor(reg)) return;

    sf::FloatRect bounds = boundsComp->bounds;
    sf::Vector2f worldPos = Physics::getWorldPos(ent, reg);

    sf::Vector2f bl = worldPos + bounds.position;
    sf::Vector2f tr = bl + bounds.size;
    bl.y *= -1.f;
    tr.y *= -1.f;

    sf::Vector2f drawPos(bl.x, tr.y);
    sf::Vector2f drawSize(tr.x - bl.x, bl.y - tr.y);

    sf::RectangleShape rect(drawSize);
    rect.setPosition(drawPos);
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineColor(sf::Color::Yellow);
    rect.setOutlineThickness(2.f);

    ev.window.draw(rect);
}

void EditorSystem::init(entt::registry& reg) {
    subscribe_global_event<GlobalClickEvent, &EditorSystem::onGlobalClick>(reg, this);
}

void EditorSystem::onGlobalClick(const GlobalClickEvent& ev) {
    if (!ev.pressed || *ev.handled) return;

    entt::entity ed = get_editor(*ev.registry);
    if (ed == entt::null) return;
    UIComp& edUi = ev.registry->get<UIComp>(ed);
    if (edUi.selfHidden || edUi._parentHidden) return;

    if (mode == EditorMode::Select || mode == EditorMode::Spawn) {
        entt::entity mainCam = entt::null;
        auto camView = ev.registry->view<MainCameraComp, CameraComp>();
        for (auto [ent, main, cam] : camView.each()) {
            mainCam = ent;
            break;
        }
        if (mainCam == entt::null) return;

        entt::entity worldEnt = Physics::getWorld(mainCam, *ev.registry);
        sf::Vector2f worldClickPos = Input::get_cam_mouse_pos(ev.pixelCoords, mainCam, *ev.registry);

        if (mode == EditorMode::Select) {
            bool end = false;
            Input::handle_mouse_event<ClickEvent, RenderableComp>(ev,
                [&](sf::Vector2f, sf::Vector2f, entt::entity ent, bool* handled) {
                    entt::entity world = Physics::getWorld(ent, *ev.registry);
                    if (world != worldEnt || end) { // if we clicked on something outside of primary world, skip everything and let it handle it
                        end = true;
                        return;
                    }

                    auto selView = ev.registry->view<EditorSelectedComp>();
                    for (auto ent : selView) {
                        ev.registry->remove<EditorSelectedComp>(ent);
                    }
                    ev.registry->emplace<EditorSelectedComp>(ent);
                    *handled = true;
                });
        } else if (mode == EditorMode::Spawn) {
            bool bad = false;
            entt::entity edWorld = Physics::getWorld(ed, *ev.registry);
            // We process before other click handlers so terminate if anything else wants to handle this
            Input::handle_mouse_event<ClickEvent, RenderableComp>(ev,
                [&](sf::Vector2f, sf::Vector2f, entt::entity, bool*) {
                    bad = true;
                }, edWorld);
            if (bad) return;

            entt::entity newEnt = ev.registry->create();
            ev.registry->emplace<PositionComp>(newEnt, worldClickPos).setParent(edWorld, newEnt, *ev.registry);
            ev.registry->emplace<RenderableComp>(newEnt, z_entity);
            ev.registry->emplace<BoundsComp>(newEnt, sf::FloatRect{{-16.f, -16.f}, {32.f, 32.f}});

            sf::Sprite sprite(tex_map["mob"]);
            sprite.setColor(sf::Color::Cyan);
            sprite.setOrigin((sf::Vector2f)tex_map["mob"].getSize() / 2.f);
            ev.registry->emplace<SpriteComp>(newEnt, sprite);

            auto selView = ev.registry->view<EditorSelectedComp>();
            for (auto ent : selView) {
                ev.registry->remove<EditorSelectedComp>(ent);
            }
            ev.registry->emplace<EditorSelectedComp>(newEnt);

            *ev.handled = true;
        }
    }
}
