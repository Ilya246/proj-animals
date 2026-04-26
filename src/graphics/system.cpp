#include <filesystem>
#include <map>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entt.hpp>

#include "core/events.hpp"
#include "graphics/components.hpp"
#include "graphics/system.hpp"
#include "graphics/events.hpp"
#include "graphics/texture.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"
#include "utility/formatters.hpp" // IWYU pragma: keep

void DrawSystem::init(entt::registry& reg) {
    subscribe_global_event<UpdateEvent, &DrawSystem::update>(reg, this);
    subscribe_local_event<SpriteComp, RenderEvent, &SpriteComp::OnRender>(reg);

    // load textures
    const std::filesystem::path textures_path("resources/textures");
    for (const auto& tex_file : std::filesystem::directory_iterator(textures_path)) {
        if (tex_file.is_regular_file()) {
            auto name = tex_file.path().stem().string();
            sf::Texture& file_texture = tex_map[name];
            if (!file_texture.loadFromFile(tex_file))
                tex_map.erase(name);
            else
                tex_name_map[&file_texture] = name;
        }
    }
}

void DrawSystem::update(const UpdateEvent& ev) {
    entt::registry& reg = *ev.registry;
    sf::RenderWindow& window = reg.ctx().get<sf::RenderWindow>();
	sf::Vector2u windowSize = window.getSize();

    // Both static to reduce heap allocation, but will mess with multithreading
    // however, this is rendering, so multithreading is probably out of the question
    // Map of map_entity->render_params
    static std::map<entt::entity, std::vector<std::tuple<entt::entity, sf::Vector2f, RenderableComp&>>> renderables;
    // Vector of cam->[z_level->entity] for drawing in order
    // This can be optimised, but that shouldn't be needed unless we're drawing really large amounts of entities
    static std::vector<std::pair<int32_t, std::pair<entt::entity, std::vector<std::pair<int32_t, entt::entity>>>>> drawables;

    window.clear(sf::Color::Black);

    // First collect all renderables and associate with map
    auto drawView = reg.view<RenderableComp, PositionComp>();
    renderables.clear();
    for (auto [entity, spr, pos] : drawView.each()) {
        auto [worldEnt, entPos] = Physics::getWorldAndPos(entity, reg);

        renderables[worldEnt].emplace_back(entity, pos.position, spr);
    }

    // Now go over each camera and render entities in its world
    // This is inefficient if we have 2 cameras in one world, but this is currently UB anyway
    drawables.clear();
    auto cameraView = reg.view<CameraComp, PositionComp>();
    for (auto [camEntity, camComp, camPosComp] : cameraView.each()) {
        // Check where this camera is and set up its view to have SFML do coordinate translation for us
        auto [worldEnt, camPos] = Physics::getWorldAndPos(camEntity, reg);
        sf::Vector2f camDrawPos = {camPos.x, camPos.y * -1.f};
        camComp.view.setCenter(camDrawPos);
        sf::Vector2f viewSize = (sf::Vector2f)windowSize / camComp.scale;
        camComp.view.setSize(viewSize);

        // Anything intersecting with those bounds should be rendered
        sf::FloatRect camBounds{camPos - viewSize * 0.5f, viewSize};

        drawables.push_back({camComp.zLevel, {camEntity, {}}});

        for (auto& [entity, entPos, spr] : renderables[worldEnt]) {
            // Check if the entity's draw-bounds overlap with the camera's vision bounds
            sf::FloatRect sprBounds = get_optional_bounds(entity, spr, reg);
            sf::FloatRect drawBounds {camBounds.position - sprBounds.size - sprBounds.position, camBounds.size + sprBounds.size};
            if (drawBounds.contains(entPos))
                drawables.back().second.second.push_back({spr.zLevel, entity});
        }
    }

    std::sort(drawables.begin(), drawables.end());
    for (auto& [camZ, camDraw] : drawables) {
        entt::entity cam = camDraw.first;
        auto draw = camDraw.second;
        std::sort(draw.begin(), draw.end());
        sf::View& view = reg.get<CameraComp>(cam).view;
        window.setView(view);

        for (auto [zLevel, entity] : draw) {
            bool set_scissor = false;
            if (auto* stencil = reg.try_get<StencilDrawComp>(entity)) {
                sf::Vector2f pos = Physics::getWorldPos(entity, reg);
                sf::FloatRect bounds = get_optional_bounds(entity, *stencil, reg);

                sf::Vector2f viewSize = view.getSize();
                sf::Vector2f viewTL = view.getCenter() - viewSize * 0.5f;

                float worldTop = pos.y + bounds.position.y + bounds.size.y;
                float worldLeft = pos.x + bounds.position.x;
                
                sf::Vector2f drawTL = {worldLeft, -worldTop};

                sf::FloatRect scissorRect;
                scissorRect.position.x = (drawTL.x - viewTL.x) / viewSize.x;
                scissorRect.position.y = (drawTL.y - viewTL.y) / viewSize.y;
                scissorRect.size.x = bounds.size.x / viewSize.x;
                scissorRect.size.y = bounds.size.y / viewSize.y;
                auto inters = scissorRect.findIntersection(sf::FloatRect({0.f, 0.f}, {1.f, 1.f}));
                if (inters.has_value()) {
                    scissorRect = *inters;
                    view.setScissor(scissorRect);
                    set_scissor = true;
                } else {
                    view.setScissor(sf::FloatRect{{0.f, 0.f}, {0.f, 0.f}});
                    set_scissor = true;
                }
                window.setView(view);
            }
            raise_local_event(reg, entity, RenderEvent{entity, &reg, &window});
            if (set_scissor) {
                view.setScissor(sf::FloatRect{{0.f, 0.f}, {1.f, 1.f}}); // Clean up mask
                window.setView(view);
            }
        }
    }
    window.display();
}

void SpriteComp::OnRender(RenderEvent& ev) {
    sf::Vector2f pos = Physics::getWorldPos(ev.ent, *ev.reg);
    pos.y *= -1.f;
    sprite.setPosition(pos);
    ev.window->draw(sprite);
}