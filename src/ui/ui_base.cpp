#include "core/events.hpp"
#include "entt/entity/fwd.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/components.hpp"
#include "ui/events.hpp"
#include "utility/geom.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

template<>
void handle_event(ShouldRenderEvent& ev, entt::entity ent, UIComp& comp, entt::registry& reg) {
    if (comp.selfHidden || comp._parentHidden) {
        *ev.cancelled = true;
        return;
    }
    if (comp._cachedStencil) {
        sf::Vector2f pos = Physics::getWorldPos(ent, reg);
        *ev.stencil = {comp._cachedStencil->position + pos, comp._cachedStencil->size};
    }
}

template<>
void handle_event(UIPropagateEvent& ev, entt::entity ent, UIComp& comp, entt::registry& reg) {
    comp.propagate(ev, ent, reg);
}

template<>
void handle_event(BoundsResizeEvent& ev, entt::entity ent, UIComp& comp, entt::registry& reg) {
    if (comp.isStencil) {
        comp.assign_stencil(ev.newBounds, reg, ent);
    }
}

void UIComp::set_hidden(bool hide, entt::registry& reg, entt::entity ent) {
    selfHidden = hide;

    UIPropagateEvent ev {
        [](entt::registry& r, entt::entity e) {
            if (auto* ui = r.try_get<UIComp>(e)) {
                if (auto* pos = r.try_get<PositionComp>(e)) {
                    if (auto* parent_ui = r.try_get<UIComp>(pos->parent)) {
                        ui->_parentHidden = parent_ui->selfHidden || parent_ui->_parentHidden;
                    }
                }
            }
        }
    };

    propagate(ev, ent, reg);
}

void UIComp::assign_stencil(std::optional<sf::FloatRect> stencil, entt::registry& reg, entt::entity ent) {
    // Convert to world-coordinates
    if (stencil) {
        sf::Vector2f coord = Physics::getWorldPos(ent, reg);
        stencil->position += coord;
    }

    UIPropagateEvent ev {
        [stencil](entt::registry& r, entt::entity e) {
            if (auto* ui = r.try_get<UIComp>(e)) {
                std::optional<sf::FloatRect> st = stencil;
                if (st) {
                    // de-shift to local coordinates
                    sf::Vector2f coord = Physics::getWorldPos(e, r);
                    st->position -= coord;
                    // Resize it for children
                    sf::FloatRect scaled = apply_dynamic_bounds(*st, ui->stencilArea);
                    *(sf::FloatRect*)&*stencil = {scaled.position + coord, scaled.size};
                }
                if (ui->isStencil && st) {
                    // Re-intersect with our own bounds if we are also a stencil provider
                    sf::FloatRect bounds = r.get<BoundsComp>(e).bounds;

                    auto inters = bounds.findIntersection(*st);
                    ui->_cachedStencil = inters.has_value() ? *inters : sf::FloatRect{{0.f, 0.f}, {0.f, 0.f}};
                } else {
                    ui->_cachedStencil = st;
                }
            }
        }
    };

    propagate(ev, ent, reg);
}

void UIComp::propagate(UIPropagateEvent& propagate, entt::entity ent, entt::registry& reg) {
    propagate.action(reg, ent);
    UIPropagateEvent copyEv(propagate);
    for (entt::entity child : children) {
        raise_local_event(reg, child, copyEv);
    }
}
