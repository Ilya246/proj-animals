#pragma once
#include <SFML/System/Vector2.hpp>
#include "core/events.hpp"
#include "core/system.hpp"
#include "graphics/components.hpp"
#include "input/events.hpp"
#include "physics/components.hpp"
#include "physics/system.hpp"

struct InputSystem : System<InputSystem> {
    private:
        virtual void init(entt::registry&) override;
        virtual int initPriority() override { return 64; };

        void update(const UpdateEvent&);
        void receiveClick(const GlobalClickEvent&);
        void receiveScroll(const GlobalScrollEvent&);
};

namespace Input {
    sf::Vector2f get_cam_mouse_pos(sf::Vector2i screenPos, entt::entity camera, entt::registry& reg);
    // guesses the camera
    std::optional<sf::Vector2f> get_world_mouse_pos(sf::Vector2i screenPos, entt::entity world, entt::registry& reg);

    // Raises TEv on entities intersecting with LComp with the mouse upon TGEv being fired
    template<typename TEv, typename LComp, typename TGEv, bool hasBounds = false>
    void handle_mouse_event(const TGEv& ev, std::function<void(sf::Vector2f, sf::Vector2f, entt::entity, bool*)> click_fun, entt::entity world = entt::null) {
        if (*ev.handled) return;

        std::map<entt::entity, sf::Vector2f> clickMap;
        auto camView = ev.registry->template view<CameraComp>();
        for (auto[entity, cam] : camView.each()) {
            auto worldEnt = Physics::getWorld(entity, *ev.registry);
            sf::Vector2f coords = Input::get_cam_mouse_pos(ev.pixelCoords, entity, *ev.registry);
            clickMap[worldEnt] = coords;
        }

        struct clickCandidate {
            entt::entity ent;
            sf::Vector2f clickPos;
            sf::Vector2f worldPos;
        };

        std::map<int32_t, clickCandidate, std::greater<>> candidates;
        auto scrollableView = ev.registry->template view<LComp, PositionComp>();
        for (auto [entity, lcomp, position] : scrollableView.each()) {
            auto[worldEnt, worldPos] = Physics::getWorldAndPos(entity, *ev.registry);

            if (world != entt::null && worldEnt != world)
                continue;

            bool nonRenderable = false;
            std::optional<sf::FloatRect> stencil = std::nullopt;
            ShouldRenderEvent shouldEv{&nonRenderable, &stencil};
            raise_local_event(*ev.registry, entity, shouldEv);

            if (nonRenderable || !clickMap.contains(worldEnt))
                continue;

            sf::Vector2f clickPos = clickMap.at(worldEnt);

            if (stencil.has_value() && !stencil->contains(clickPos))
                continue;

            sf::FloatRect bounds;
            if constexpr (hasBounds)
                bounds = get_optional_bounds(entity, lcomp, *ev.registry);
            else
                bounds = ev.registry->template get<BoundsComp>(entity).bounds;

            bounds.position += worldPos;
            if (bounds.contains(clickPos)) {
                int32_t zLevel = ev.registry->template get<RenderableComp>(entity).zLevel;
                candidates[zLevel] = {entity, clickPos, worldPos};
            }
        }

        for (auto [z, cand] : candidates) {
            click_fun(cand.clickPos - cand.worldPos, cand.clickPos, cand.ent, ev.handled);
            if (*ev.handled)
                break;
        }
    }
}
