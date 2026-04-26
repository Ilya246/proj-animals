#pragma once
#include "input/components.hpp"
#include "serialization/serialization.hpp"
#include "ui/components.hpp"
#include "graphics/components.hpp"
#include "physics/components.hpp"
#include "core/components.hpp"
#include "ui/font.hpp"
#include <entt/entt.hpp>
#include <string>
#include <functional>

struct UIBuilder {
    entt::registry& reg;
    entt::entity ent;

    UIBuilder(entt::entity ent, entt::registry& r) : reg(r), ent(ent) {};

    UIBuilder(entt::registry& r, entt::entity parent, const std::string& name = "") : reg(r), ent(r.create()) {
        if (!name.empty()) set_ent_name(ent, r, name);
        r.emplace<PositionComp>(ent, sf::Vector2f(0.f, 0.f), parent);
        r.emplace<RenderableComp>(ent, r.get<RenderableComp>(parent).zLevel + 1);
        r.emplace<BoundsComp>(ent);
        r.emplace<NonSerializableComp>(ent);

        if (r.valid(parent)) {
            if (auto* alloc = r.try_get<UIFullAllocatorComp>(parent)) {
                alloc->children.push_back(ent);
            } else if (auto* lay = r.try_get<UILayoutComp>(parent)) {
                lay->children.push_back(ent);
            } else {
                r.emplace<UIFullAllocatorComp>(parent).children.push_back(ent);
            }
        }
    }

    entt::entity get() const { return ent; }
    operator entt::entity() const { return ent; }

    static UIBuilder makeWorld(entt::registry& r, const std::string& name = "") {
        UIBuilder world = {r.create(), r};
        set_ent_name(world.ent, r, name);
        world.emplace<PositionComp>(sf::Vector2f(0.f, 0.f), world);
        world.emplace<RenderableComp>(z_ui);
        world.emplace<BoundsComp>();
        world.emplace<UIFullAllocatorComp>();

        entt::entity cam = r.create();
        set_ent_name(cam, r, name + ": Camera");
        r.emplace<PositionComp>(cam, sf::Vector2f(0.f, 0.f), world);
        r.emplace<CameraComp>(cam, 1.f, z_ui);

        world.emplace<UIScreenComp>(cam);
        return world;
    }

    UIBuilder child(const std::string& name = "") {
        return UIBuilder(reg, ent, name);
    }

    UIBuilder& posAnchor(const sf::FloatRect& bounds) {
        reg.emplace<UIAnchorComp>(ent, bounds);
        return *this;
    }

    UIBuilder& posAnchor(float pX, float pY, float sX, float sY) {
        return posAnchor({{pX, pY}, {sX, sY}});
    }

    UIBuilder& posFill() {
        reg.emplace<UIFillComp>(ent);
        return *this;
    }

    UIBuilder& allocatorFull() {
        reg.emplace<UIFullAllocatorComp>(ent);
        return *this;
    }

    UIBuilder& allocatorLayout(UILayoutMode mode, float padding = 0.f, float spacing = 0.f) {
        auto& lay = reg.emplace<UILayoutComp>(ent);
        lay.mode = mode;
        lay.padding = padding;
        lay.spacing = spacing;
        return *this;
    }

    UIBuilder& zIndex(int32_t z) {
        reg.get<RenderableComp>(ent).zLevel = z;
        return *this;
    }

    UIBuilder& rect(sf::Color fill, sf::Color border, float thickness = 2.f) {
        reg.emplace<UIRectComp>(ent, fill, border, thickness);
        return *this;
    }

    UIBuilder& text(const std::string& str, const std::string& font, unsigned int size, sf::Color color = sf::Color::White, bool wrap = false) {
        auto& t = reg.emplace<TextComp>(ent, sf::Text(font_map[font], str, size));
        t.text.setFillColor(color);
        t.wrap = wrap;
        return *this;
    }

    UIBuilder& button(std::function<void(ClickEvent&)>&& cb) {
        reg.emplace<ButtonComp>(ent, cb);
        reg.emplace<ClickListenerComp>(ent);
        return *this;
    }

    UIBuilder& stencil() {
        reg.emplace<StencilDrawComp>(ent);
        return *this;
    }

    template<typename Comp, typename... Args>
    UIBuilder& emplace(Args&&... args) {
        reg.emplace<Comp>(ent, std::forward<Args>(args)...);
        return *this;
    }
};
