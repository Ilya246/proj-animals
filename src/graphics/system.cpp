#include <SFML/Graphics/RenderWindow.hpp>
#include <entt/entt.hpp>

#include "core/events.hpp"
#include "entt/entity/fwd.hpp"
#include "graphics/components.hpp"
#include "graphics/system.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "world/components.hpp"

REGISTER_SYSTEM(DrawSystem);

void DrawSystem::init(entt::registry& reg) {
    subscribeGlobalEvent<UpdateEvent, &DrawSystem::update>(reg, this);
}

void DrawSystem::update(const UpdateEvent& ev) {
    entt::registry& reg = *ev.registry;
    sf::RenderWindow& window = reg.ctx().get<sf::RenderWindow>();
	sf::Vector2u windowSize = window.getSize();

    // Rendering system: update sprite positions and draw
    window.clear(sf::Color::Black);
    auto cameraView = reg.view<PositionComp, CameraComp>();
    for (auto [camEntity, camPosComp, camComp] : cameraView.each()) {
        auto [worldEnt, camPos] = Physics::getWorldPos(camEntity, reg);
        camPos.y *= -1.f; // translate to draw coordinates
        camComp.view.setCenter(camPos);
        camComp.view.setSize((sf::Vector2f)windowSize / camComp.scale);
        window.setView(camComp.view);

        // Render the world first
        if (TileMapComp* mapComp = reg.try_get<TileMapComp>(worldEnt)) {
            sf::RenderStates states;
            states.texture = &*mapComp->texture;
            window.draw(mapComp->vertices, states);
        }

        auto drawView = reg.view<PositionComp, SpriteComp>();
        for (auto [entity, pos, spr] : drawView.each()) {
            sf::Vector2f spritePos = Physics::worldPos(entity, reg);
            spritePos.y *= -1.f;
            spr.sprite.setPosition(spritePos);
            window.draw(spr.sprite);
        }
    }
    window.display();
}