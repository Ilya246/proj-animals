#include <SFML/Graphics/RectangleShape.hpp>
#include "editor/system.hpp"
#include "editor/components.hpp"
#include "graphics/components.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "input/system.hpp"
#include "graphics/texture.hpp"

void EditorSelectedComp::OnRender(RenderEvent& ev) {
    auto* boundsComp = ev.reg->try_get<BoundsComp>(ev.ent);
    if (!boundsComp) return;

    sf::FloatRect bounds = boundsComp->bounds;
    sf::Vector2f worldPos = Physics::getWorldPos(ev.ent, *ev.reg);

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

    ev.window->draw(rect);
}

void EditorSystem::init(entt::registry& reg) {
    subscribe_global_event<GlobalClickEvent, &EditorSystem::onGlobalClick>(reg, this);
    subscribe_global_event<KeyPressEvent, &EditorSystem::onKeyPress>(reg, this);
    subscribe_local_event<EditorSelectedComp, RenderEvent, &EditorSelectedComp::OnRender>(reg);

    // Put pointer in context so UI callback buttons can mutate the state mode
    reg.ctx().emplace<EditorSystem*>(this);
}

void EditorSystem::onKeyPress(const KeyPressEvent& ev) {
    if (ev.key == sf::Keyboard::Key::F3) {
        editorActive = !editorActive;
    }
}

void EditorSystem::onGlobalClick(const GlobalClickEvent& ev) {
    if (!ev.pressed || !editorActive) return;

    if (mode == EditorMode::Select || mode == EditorMode::Spawn) {
        entt::entity mainCam = entt::null;
        auto camView = ev.registry->view<MainCameraComp, CameraComp>();
        for (auto [ent, main, cam] : camView.each()) {
            mainCam = ent;
            break;
        }
        if (mainCam == entt::null) return;

        auto worldEnt = Physics::getWorld(mainCam, *ev.registry);
        sf::Vector2f worldClickPos = Input::get_cam_mouse_pos(ev.pixelCoords, mainCam, *ev.registry);

        if (mode == EditorMode::Select) {
            std::map<int32_t, entt::entity, std::greater<int32_t>> hits;
            auto view = ev.registry->view<RenderableComp, BoundsComp>();
            for (auto[ent, rend, boundsComp] : view.each()) {
                if (Physics::getWorld(ent, *ev.registry) != worldEnt) continue;

                sf::Vector2f pos = Physics::getWorldPos(ent, *ev.registry);
                sf::FloatRect bounds = boundsComp.bounds;
                bounds.position += pos;

                if (bounds.contains(worldClickPos)) {
                    hits[rend.zLevel] = ent;
                }
            }

            auto selView = ev.registry->view<EditorSelectedComp>();
            for (auto ent : selView) {
                ev.registry->remove<EditorSelectedComp>(ent);
            }

            if (!hits.empty()) {
                ev.registry->emplace<EditorSelectedComp>(hits.begin()->second);
            }
        } else if (mode == EditorMode::Spawn) {
            entt::entity newEnt = ev.registry->create();
            ev.registry->emplace<PositionComp>(newEnt, worldClickPos, worldEnt);
            ev.registry->emplace<RenderableComp>(newEnt, z_entity);
            ev.registry->emplace<BoundsComp>(newEnt, sf::FloatRect{{-16.f, -16.f}, {32.f, 32.f}});

            if (tex_map.contains("mob")) {
                sf::Sprite sprite(tex_map["mob"]);
                sprite.setColor(sf::Color::Cyan);
                sprite.setOrigin((sf::Vector2f)tex_map["mob"].getSize() / 2.f);
                ev.registry->emplace<SpriteComp>(newEnt, sprite);
            }

            auto selView = ev.registry->view<EditorSelectedComp>();
            for (auto ent : selView) {
                ev.registry->remove<EditorSelectedComp>(ent);
            }
            ev.registry->emplace<EditorSelectedComp>(newEnt);
        }
    }
}
