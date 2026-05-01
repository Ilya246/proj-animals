#include "physics/components.hpp"
#include "physics/system.hpp"
#include "ui/components.hpp"

void UIComp::OnTryDraw(ShouldRenderEvent& ev) {
    if (selfHidden || _parentHidden) {
        *ev.cancelled = true;
        return;
    }
    if (_cachedStencil.has_value()) {
        sf::Vector2f pos = Physics::getWorldPos(ev.ent, *ev.reg);
        *ev.stencil = {_cachedStencil->position + pos, _cachedStencil->size};
    }
}

void UIComp::OnPropagate(UIPropagateEvent& ev) {
    propagate(ev, ev.entity, *ev.registry);
}

void UIComp::OnResize(BoundsResizeEvent& ev) {
    if (isStencil) {
        sf::FloatRect new_stencil = ev.newBounds;

        // Intersect with parent stencil if one exists
        if (auto* posComp = ev.registry->try_get<PositionComp>(ev.entity)) {
            if (auto* parentUI = ev.registry->try_get<UIComp>(posComp->parent)) {
                if (parentUI->_cachedStencil.has_value()) {
                    auto inters = new_stencil.findIntersection(*parentUI->_cachedStencil);
                    new_stencil = inters.has_value() ? *inters : sf::FloatRect{{0.f, 0.f}, {0.f, 0.f}};
                }
            }
        }

        assign_stencil(new_stencil, *ev.registry, ev.entity);
    }
}

void UIComp::set_hidden(bool hide, entt::registry& reg, entt::entity ent) {
    selfHidden = hide;

    UIPropagateEvent ev {
        &reg, ent,
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
    _cachedStencil = stencil;

    UIPropagateEvent ev {
        &reg, ent,
        [stencil](entt::registry& r, entt::entity e) {
            if (auto* ui = r.try_get<UIComp>(e)) {
                if (ui->isStencil && stencil.has_value()) {
                    // Re-intersect with our own bounds if we are also a stencil provider
                    sf::FloatRect bounds = r.get<BoundsComp>(e).bounds;

                    auto inters = bounds.findIntersection(*stencil);
                    ui->_cachedStencil = inters.has_value() ? *inters : sf::FloatRect{{0.f, 0.f}, {0.f, 0.f}};
                } else {
                    ui->_cachedStencil = stencil;
                }
            }
        }
    };

    propagate(ev, ent, reg);
}

void UIComp::propagate(UIPropagateEvent& propagate, entt::entity ent, entt::registry& reg) {
    propagate.action(reg, ent);
    for (entt::entity child : children) {
        raise_local_event(reg, child, propagate);
    }
}
