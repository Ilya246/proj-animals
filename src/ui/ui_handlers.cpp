#include "physics/components.hpp"
#include "ui/components.hpp"

template<>
void handle_event(UISizeAllocatedEvent& ev, entt::entity ent, UIFillComp&, entt::registry& reg) {
    reg.get<PositionComp>(ent).position = ev.bounds.position;
    reg.get<BoundsComp>(ent).resize({zeroVec, ev.bounds.size}, ent, reg);
}

template<>
void handle_event(UISizeAllocatedEvent& ev, entt::entity ent, UIAnchorComp& comp, entt::registry& reg) {
    sf::FloatRect alloc = ev.bounds;
    sf::FloatRect scaledBounds = apply_dynamic_bounds(alloc, comp.bounds);
    reg.get<PositionComp>(ent).position = scaledBounds.position;
    reg.get<BoundsComp>(ent).resize({zeroVec, scaledBounds.size}, ent, reg);
}

template<>
void handle_event(UISizeAllocatedEvent&, entt::entity ent, UIAbsoluteBoundsComp& comp, entt::registry& reg) {
    reg.get<BoundsComp>(ent).resize(comp.bounds, ent, reg);
}

template<>
void handle_event(UIQueryChildEvent& ev, entt::entity, UILayoutConstraintComp& comp, entt::registry&) {
    ev.constraints.minWidth = std::max(ev.constraints.minWidth, comp.minWidth);
    ev.constraints.minHeight = std::max(ev.constraints.minHeight, comp.minHeight);
    if (comp.expandX) { ev.constraints.expandX = true; ev.constraints.weightX = std::max(ev.constraints.weightX, comp.weightX); }
    if (comp.expandY) { ev.constraints.expandY = true; ev.constraints.weightY = std::max(ev.constraints.weightY, comp.weightY); }
}
