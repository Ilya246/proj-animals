#include "physics/components.hpp"
#include "ui/components.hpp"

void UIFillComp::OnAllocate(UISizeAllocatedEvent& ev) {
    ev.registry->get<PositionComp>(ev.entity).position = ev.bounds.position;
    ev.registry->get<BoundsComp>(ev.entity).resize({zeroVec, ev.bounds.size}, ev.entity, *ev.registry);
}

void UIAnchorComp::OnAllocate(UISizeAllocatedEvent& ev) {
    sf::FloatRect alloc = ev.bounds;
    sf::FloatRect scaledBounds = apply_dynamic_bounds(alloc, bounds);
    ev.registry->get<PositionComp>(ev.entity).position = scaledBounds.position;
    ev.registry->get<BoundsComp>(ev.entity).resize({zeroVec, scaledBounds.size}, ev.entity, *ev.registry);
}

void UIAbsoluteBoundsComp::OnAllocate(UISizeAllocatedEvent& ev) {
    ev.registry->get<BoundsComp>(ev.entity).resize(bounds, ev.entity, *ev.registry);
}
