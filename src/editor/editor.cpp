#include <SFML/Graphics/RectangleShape.hpp>
#include "core/events.hpp"
#include "editor/system.hpp"
#include "editor/components.hpp"
#include "graphics/components.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "input/system.hpp"
#include "graphics/texture.hpp"
#include "ui/builder.hpp"
#include "ui/components.hpp"
#include "ui/system.hpp"

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
    BoundsComp* boundsComp = reg.try_get<BoundsComp>(ent);
    if (!boundsComp) return;
    if (!haveEditor(reg)) return;
    EditorSystem& sys = reg.ctx().get<EditorSystem>();
    if (sys.mode != EditorMode::Select) return;

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
    subscribe_global_event<UpdateEvent, &EditorSystem::update>(reg, this);
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
            ev.registry->emplace<PositionComp>(newEnt, worldClickPos).setParent(worldEnt, newEnt, *ev.registry);
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

void EditorSystem::update(const UpdateEvent& ev) {
    entt::registry& reg = *ev.registry;

    entt::entity selectedEnt = entt::null;
    for (auto [e, esc] : reg.view<EditorSelectedComp>().each()) {
        selectedEnt = e;
        break;
    }

    auto view = reg.view<ComponentEditorUIComp>();
    for (auto [entity, ce] : view.each()) {
        if (ce.targetEntity != selectedEnt) {
            ce.targetEntity = selectedEnt;
            ce.selectedComponent = "";
        }

        if (!reg.valid(ce.paramListContainer)) continue;

        if (ce.selectedComponent != ce.lastSelectedComponent) {
            auto& containerUI = reg.get<UIComp>(ce.paramListContainer);
            for (auto child : containerUI.children) {
                queue_delete(child, reg);
            }
            containerUI.children.clear();
            ce.paramBoxes.clear();

            if (!ce.selectedComponent.empty() && reg.valid(ce.targetEntity)) {
                auto nodeOpt = ComponentSerializer::serialize(ce.selectedComponent, reg, ce.targetEntity);
                if (nodeOpt && nodeOpt->IsMap()) {
                    YAML::Node node = *nodeOpt;
                    for (auto it = node.begin(); it != node.end(); ++it) {
                        std::string key = it->first.as<std::string>();

                        UIBuilder rowContainer(reg, ce.paramListContainer, "Param Row " + key);
                        rowContainer.allocatorLayout(UILayoutMode::Horizontal, 0.f, 2.f)
                                    .posFill()
                                    .constraint(0.f, 24.f, true, false);

                        rowContainer.child(key + ": Label")
                            .text(key + ":", "hack", 12)
                            .posFill()
                            .constraint(80.f, 20.f, false, false);

                        UIBuilder tb = rowContainer.child(key + ": Textbox")
                            .textbox("hack", 12, sf::Color::White,
                            [compName = ce.selectedComponent, key](std::string_view text, entt::entity, entt::registry& r) {
                                auto v = r.view<ComponentEditorUIComp>();
                                if (v.empty()) return;
                                auto& compEd = r.get<ComponentEditorUIComp>(v.front());
                                if (!r.valid(compEd.targetEntity)) return;

                                auto nOpt = ComponentSerializer::serialize(compName, r, compEd.targetEntity);
                                if (!nOpt) return;
                                YAML::Node n = *nOpt;
                                try {
                                    n[key] = YAML::Load(std::string(text));
                                    ComponentSerializer::deserialize(compName, n, compEd.targetEntity, &r);
                                } catch (const std::exception& e) {
                                    std::cout << "[Editor] Failed to parse parameter update: " << e.what() << "\n";
                                }
                            })
                            .posFill()
                            .rect(sf::Color(20, 20, 20), sf::Color(100, 100, 100), 1.f)
                            .constraint(60.f, 20.f, true, false);

                        ce.paramBoxes[key] = tb;
                    }
                } else {
                    ce.selectedComponent = "";
                }
            }

            ce.lastSelectedComponent = ce.selectedComponent;
            UI::rebuild(ce.paramListContainer, reg);
        } else if (!ce.selectedComponent.empty() && reg.valid(ce.targetEntity)) {
            // Update live values every frame
            auto nodeOpt = ComponentSerializer::serialize(ce.selectedComponent, reg, ce.targetEntity);
            if (!nodeOpt) {
                ce.selectedComponent = ""; // Component got removed
                continue;
            }
            YAML::Node node = *nodeOpt;
            auto& uiSys = reg.ctx().get<UISystem>();

            if (node.IsMap()) {
                for (auto it = node.begin(); it != node.end(); ++it) {
                    std::string key = it->first.as<std::string>();
                    if (!ce.paramBoxes.contains(key)) continue;

                    entt::entity tbEnt = ce.paramBoxes[key];
                    if (tbEnt == uiSys.activeTextbox) continue; // Skip live updates for fields currently focused

                    std::string valStr;
                    if (it->second.IsScalar()) valStr = it->second.as<std::string>();
                    else {
                        valStr = YAML::Dump(it->second);
                        if (!valStr.empty() && valStr.back() == '\n') valStr.pop_back();
                    }

                    auto& tbComp = reg.get<TextBoxComp>(tbEnt);
                    if (tbComp.content != valStr) {
                        tbComp.content = valStr;
                        if (auto* bounds = reg.try_get<BoundsComp>(tbEnt)) {
                            tbComp.stringChanged(bounds->bounds.size.x);
                        } else {
                            tbComp.stringChanged(0.f);
                        }
                    }
                }
            }
        }
    }
}
