#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <entt/entt.hpp>

#include "core/events.hpp"
#include "entt/entity/fwd.hpp"
#include "graphics/components.hpp"
#include "graphics/system.hpp"
#include "graphics/events.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"

void DrawSystem::init(entt::registry& reg) {
    subscribeGlobalEvent<UpdateEvent, &DrawSystem::update>(reg, this);
    subscribeLocalEvent<SpriteComp, RenderEvent, &SpriteComp::OnRender>(reg);
}

void DrawSystem::update(const UpdateEvent& ev) {
    entt::registry& reg = *ev.registry;
    sf::RenderWindow& window = reg.ctx().get<sf::RenderWindow>();
	sf::Vector2u windowSize = window.getSize();

    std::vector<std::pair<int32_t, entt::entity>> drawables;
    // Rendering system: update sprite positions and draw
    window.clear(sf::Color::Black);
    auto cameraView = reg.view<CameraComp, PositionComp>();
    for (auto [camEntity, camComp, camPosComp] : cameraView.each()) {
        auto [worldEnt, camPos] = Physics::getWorldPos(camEntity, reg);
        sf::Vector2f camDrawPos = {camPos.x, camPos.y * -1.f}; // translate to draw coordinates
        camComp.view.setCenter(camDrawPos);
        sf::Vector2f viewSize = (sf::Vector2f)windowSize / camComp.scale;
        camComp.view.setSize(viewSize);
        window.setView(camComp.view);

        sf::FloatRect camBounds{camPos - viewSize * 0.5f, viewSize};

        drawables.clear();
        auto drawView = reg.view<RenderableComp, PositionComp>();
        for (auto [entity, spr, pos] : drawView.each()) {
            auto [thisWorld, spritePos] = Physics::getWorldPos(entity, reg);
            if (thisWorld != worldEnt)
                continue;

            // larger size = can draw it from farther out towards bottomleft
            // larger pos = can draw it from farther out topright
            sf::FloatRect drawBounds {camBounds.position - spr.Bounds.size + spr.Bounds.position, camBounds.size + spr.Bounds.size};
            if (!drawBounds.contains(spritePos))
                continue;

            drawables.push_back({spr.zLevel, entity});
        }

        std::sort(drawables.begin(), drawables.end());

        for (auto [zLevel, entity] : drawables) {
            raiseLocalEvent(reg, entity, RenderEvent(entity, &reg, &window));
        }
    }
    window.display();
}

void SpriteComp::OnRender(RenderEvent& ev) {
    sf::Vector2f pos = Physics::worldPos(ev.ent, *ev.reg);
    pos.y *= -1.f;
    sprite.setPosition(pos);
    ev.window->draw(sprite);
}