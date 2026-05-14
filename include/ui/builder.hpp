#pragma once
#include "input/components.hpp"
#include "serialization/serialization.hpp"
#include "ui/components.hpp"
#include "graphics/components.hpp"
#include "physics/components.hpp"
#include "core/components.hpp"
#include "ui/font.hpp"
#include "utility/constants.hpp"
#include "utility/geom.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <entt/entt.hpp>
#include <string>
#include <functional>

struct UIBuilder {
    entt::registry& reg;
    entt::entity ent;

    UIBuilder(entt::entity ent, entt::registry& r) : reg(r), ent(ent) {};

    UIBuilder(entt::registry& r, entt::entity parent, const std::string& name = "") : reg(r), ent(r.create()) {
        if (!name.empty()) set_ent_name(ent, r, name);
        r.emplace<PositionComp>(ent, sf::Vector2f(0.f, 0.f)).setParent(parent, ent, reg);
        int32_t parent_z = z_ui;
        if (auto* parentRender = r.try_get<RenderableComp>(parent))
            parent_z = parentRender->zLevel;
        r.emplace<RenderableComp>(ent, parent_z + 1);
        r.emplace<BoundsComp>(ent);
        r.emplace<NonSerializableComp>(ent);
        UIComp& self_ui = r.emplace<UIComp>(ent);

        if (r.valid(parent)) {
            if (auto* ui = r.try_get<UIComp>(parent)) {
                ui->children.push_back(ent);
                self_ui._parentHidden = ui->selfHidden;
            }
        }
    }

    entt::entity get() const { return ent; }
    operator entt::entity() const { return ent; }

    template<typename Comp, typename... Args>
    UIBuilder& emplace(Args&&... args) {
        reg.emplace<Comp>(ent, std::forward<Args>(args)...);
        return *this;
    }

    template<typename Comp, typename... Args>
    UIBuilder& ensure(Args&&... args) {
        if (!reg.try_get<Comp>(ent))
            reg.emplace<Comp>(ent, std::forward<Args>(args)...);
        return *this;
    }

    static UIBuilder makeWorld(entt::registry& r, const std::string& name = "") {
        UIBuilder world = {r.create(), r};
        set_ent_name(world.ent, r, name);
        world.emplace<PositionComp>(sf::Vector2f(0.f, 0.f));
        r.get<PositionComp>(world).setParent(world, world, r);
        world.emplace<BoundsComp>();
        world.emplace<UIFullAllocatorComp>();
        world.emplace<UIComp>();

        entt::entity cam = r.create();
        set_ent_name(cam, r, name + ": Camera");
        r.emplace<PositionComp>(cam, sf::Vector2f(0.f, 0.f)).setParent(world, cam, r);
        r.emplace<CameraComp>(cam, 1.f, z_ui);

        world.emplace<UIScreenComp>(cam);
        return world;
    }

    UIBuilder child(const std::string& name = "") {
        return UIBuilder(reg, ent, name);
    }

    UIBuilder& posAnchor(const DynamicBounds& bounds) {
        return emplace<UIAnchorComp>(bounds);
    }

    UIBuilder& posAnchor(float pX, float pY, float sX, float sY, std::array<bool, 4> fractionMode = {true, true, true, true}) {
        return posAnchor({pX, pY, sX, sY, fractionMode});
    }

    UIBuilder& posFill() {
        return emplace<UIFillComp>();
    }

    UIBuilder& posAbsolute(sf::FloatRect bounds) {
        reg.get<PositionComp>(ent).position = bounds.position;
        return emplace<UIAbsoluteBoundsComp>(sf::FloatRect{zeroVec, bounds.size});
    }

    UIBuilder& allocatorFull() {
        return emplace<UIFullAllocatorComp>();
    }

    UIBuilder& allocatorLayout(UILayoutMode mode, float padding = 0.f, float spacing = 0.f, DynamicBounds bounds = DynamicBounds::full, bool invertAnchor = false, bool allowOverflow = false) {
        return emplace<UILayoutComp>(mode, padding, spacing, bounds, invertAnchor, allowOverflow);
    }

    UIBuilder& allocatorTile(sf::Vector2f tileSize, float padding = 0.f, float spacing = 0.f, DynamicBounds bounds = DynamicBounds::full) {
        return emplace<UITileLayoutComp>(tileSize, padding, spacing, bounds);
    }

    UIBuilder& scrollable(DynamicBounds bounds, sf::Color innerColor, sf::Color outlineColor, sf::Color barColor, float outlineThickness = 2.f, float scrollMul = 25.f) {
        return emplace<UIScrollAreaComp>(bounds, innerColor, outlineColor, barColor, outlineThickness, scrollMul).ensure<ScrollListenerComp>();
    }

    UIBuilder& constraint(float minW, float minH, bool expX = false, bool expY = false, float wX = 1.f, float wY = 1.f) {
        return emplace<UILayoutConstraintComp>(minW, minH, expX, expY, wX, wY);
    }

    UIBuilder& zIndex(int32_t z) {
        reg.get<RenderableComp>(ent).zLevel = z;
        return *this;
    }

    UIBuilder& rect(sf::Color fill, sf::Color border, float thickness = 2.f) {
        return emplace<UIRectComp>(fill, border, thickness);
    }

    UIBuilder& text(const std::string& str, const std::string& font, unsigned int size, sf::Color color = sf::Color::White, bool wrap = true) {
        TextComp& t = reg.emplace<TextComp>(ent, sf::Text(font_map[font], str, size));
        t.text.setFillColor(color);
        t.wrap = wrap;
        return *this;
    }

    UIBuilder& textbox(const std::string& font, unsigned int size, sf::Color color = sf::Color::White, std::function<void(std::string_view, entt::entity, entt::registry&)>&& cb = {}) {
        TextBoxComp& tb = reg.emplace<TextBoxComp>(ent, sf::Text(font_map[font], "", size));
        tb.text.setFillColor(color);
        tb.onEnter = std::move(cb);
        return ensure<ClickListenerComp>();
    }

    // Gives us a text child entity.
    // NOTE: also give us allocatorFull().
    UIBuilder& childText(const std::string& str, const std::string& font, unsigned int size, sf::Color color = sf::Color::White, bool wrap = true) {
        UIBuilder{reg, ent, "Text"}
            .text(str, font, size, color, wrap)
            .posFill();

        return allocatorFull();
    }

    // Gives us a child textbox.
    // Returns a builder for the textbox.
    UIBuilder childTextbox(const std::string& font, unsigned int size, sf::Color color = sf::Color::White, std::function<void(std::string_view, entt::entity, entt::registry&)>&& cb = {}) {
        allocatorFull();

        return UIBuilder{reg, ent, "TextBox"}
            .textbox(font, size, color, std::move(cb))
            .posFill();
    }

    UIBuilder& button(std::function<void(ClickEvent&, entt::entity, entt::registry&)>&& cb) {
        return emplace<ButtonComp>(cb).ensure<ClickListenerComp>();
    }

    UIBuilder& draggable(std::optional<DynamicBounds> bounds = {}) {
        return emplace<DraggableComp>(bounds).ensure<ClickListenerComp>();
    }

    UIBuilder& draggable(float pX, float pY, float sX, float sY, std::array<bool, 4> fractionMode = {true, true, true, true}) {
        return draggable(DynamicBounds{pX, pY, sX, sY, fractionMode});
    }

    UIBuilder& stencil(DynamicBounds area = DynamicBounds::full) {
        UIComp& ui = reg.get<UIComp>(ent);
        ui.isStencil = true;
        ui.stencilArea = area;
        return *this;
    }

    UIBuilder& hide() {
        reg.get<UIComp>(ent).set_hidden(true, reg, ent);
        return *this;
    }

    UIBuilder& buttonToggled(sf::Keyboard::Key key) {
        return emplace<ButtonToggledUIComp>(key);
    }

    UIBuilder& toggleButton(bool initial, sf::Color onColor, sf::Color offColor, std::function<void(bool, entt::entity, entt::registry&)>&& cb) {
        return emplace<ToggleButtonComp>(initial, std::move(cb), std::vector<entt::entity>{}, onColor, offColor).ensure<ClickListenerComp>();
    }

    UIBuilder& tooltip(const std::string& text, float threshold = 0.5f) {
        return emplace<HoverListenerComp>().emplace<TooltipProviderComp>(text, threshold);
    }

    UIBuilder& dropdownTrigger(std::function<entt::entity(entt::registry&, entt::entity, entt::entity)> builder, bool openOnHover = false, float hoverThreshold = 0.5f) {
        return emplace<DropdownTriggerComp>(std::move(builder), entt::null, openOnHover, hoverThreshold)
               .ensure<ClickListenerComp>()
               .ensure<HoverListenerComp>();
    }
};
