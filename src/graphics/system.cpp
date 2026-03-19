#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entt.hpp>
#include <map>

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

    // Both static to reduce heap allocation, but will mess with multithreading
    // however, this is rendering, so multithreading is probably out of the question
    // Map of map_entity->render_params
    static std::map<entt::entity, std::vector<std::tuple<entt::entity, sf::Vector2f, RenderableComp&>>> renderables;
    // Vector of z_level->entity for drawing in order
    // This can be optimised, but that shouldn't be needed unless we're drawing really large amounts of entities
    static std::vector<std::pair<int32_t, entt::entity>> drawables;

    window.clear(sf::Color::Black);

    // First collect all renderables and associate with map
    auto drawView = reg.view<RenderableComp, PositionComp>();
    renderables.clear();
    for (auto [entity, spr, pos] : drawView.each()) {
        auto [worldEnt, entPos] = Physics::getWorldPos(entity, reg);

        renderables[worldEnt].emplace_back(entity, pos.position, spr);
    }

    // Now go over each camera and render entities in its world
    // This is inefficient if we have 2 cameras in one world, but this is currently UB anyway
    auto cameraView = reg.view<CameraComp, PositionComp>();
    for (auto [camEntity, camComp, camPosComp] : cameraView.each()) {
        // Check where this camera is and set up its view to have SFML do coordinate translation for us
        auto [worldEnt, camPos] = Physics::getWorldPos(camEntity, reg);
        sf::Vector2f camDrawPos = {camPos.x, camPos.y * -1.f};
        camComp.view.setCenter(camDrawPos);
        sf::Vector2f viewSize = (sf::Vector2f)windowSize / camComp.scale;
        camComp.view.setSize(viewSize);
        window.setView(camComp.view);

        // Anything intersecting with those bounds should be rendered
        sf::FloatRect camBounds{camPos - viewSize * 0.5f, viewSize};

        drawables.clear();
        for (auto [entity, entPos, spr] : renderables[worldEnt]) {
            // Check if the entity's draw-bounds overlap with the camera's vision bounds
            sf::FloatRect drawBounds {camBounds.position - spr.Bounds.size + spr.Bounds.position, camBounds.size + spr.Bounds.size};
            if (drawBounds.contains(entPos))
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