#pragma once
#include <SFML/Graphics/Color.hpp>
#include <cstdint>
#include <rfl.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include "graphics/texture.hpp"
#include "ui/font.hpp"

struct SfmlVector2fSerializationHelper {
    float x;
    float y;

    static SfmlVector2fSerializationHelper from_class(const sf::Vector2f& v) noexcept {
        return SfmlVector2fSerializationHelper{v.x, v.y};
    }

    sf::Vector2f to_class() const noexcept {
        return sf::Vector2f(x, y);
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::Vector2f, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType,
                          sf::Vector2f, SfmlVector2fSerializationHelper> {};
}

struct SfmlAngleHelper {
    float deg;

    static SfmlAngleHelper from_class(const sf::Angle& a) noexcept {
        return {a.asDegrees()};
    }

    sf::Angle to_class() const noexcept {
        return sf::degrees(deg);
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::Angle, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType,
                          sf::Angle, SfmlAngleHelper> {};
}

struct SfmlColorHelper {
    uint32_t col;

    static SfmlColorHelper from_class(const sf::Color& col) noexcept {
        return {col.toInteger()};
    }

    sf::Color to_class() const noexcept {
        return sf::Color(col);
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::Color, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType,
                          sf::Color, SfmlColorHelper> {};
}

struct SfmlTextureHelper {
    std::string texture;
 
    static SfmlTextureHelper from_class(const sf::Texture& tex) noexcept {
        return SfmlTextureHelper{
            tex_name_map[&tex]
        };
    }

    const sf::Texture& to_class() const noexcept {
        return tex_map[texture];
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, const sf::Texture&, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          const sf::Texture&, SfmlTextureHelper> {};
}

struct SfmlSpriteHelper {
    std::string texture;
    sf::Vector2f position{0.f, 0.f};
    sf::Angle rotation;
    sf::Vector2f origin{0.f, 0.f};
    sf::Vector2f scale{1.f, 1.f};
    sf::Color color{sf::Color::White};
    
    static SfmlSpriteHelper from_class(const sf::Sprite& sprite) noexcept {
        SfmlSpriteHelper helper;
        helper.texture = tex_name_map[&sprite.getTexture()];
        helper.position = sprite.getPosition();
        helper.rotation = sprite.getRotation();
        helper.scale = sprite.getScale();
        helper.color = sprite.getColor();
        helper.origin = sprite.getOrigin();
        return helper;
    }

    sf::Sprite to_class() const noexcept {
        sf::Sprite sprite(tex_map[texture]);
        sprite.setPosition(position);
        sprite.setRotation(rotation);
        sprite.setOrigin(origin);
        sprite.setScale(scale);
        sprite.setColor(color);
        return sprite;
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::Sprite, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          sf::Sprite, SfmlSpriteHelper> {};
}

struct SfmlViewHelper {
    static SfmlViewHelper from_class(const sf::View&) noexcept {
        return {};
    }

    sf::View to_class() const noexcept {
        return {};
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::View, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          sf::View, SfmlViewHelper> {};
}

struct SfmlTextHelper {
    std::string font;
    std::string text;
    unsigned int size;
    float letterSpacing;
    float lineSpacing;
    uint32_t style;
    sf::Color fillColor;
    sf::Color outlineColor;
    float outlineThickness;

    static SfmlTextHelper from_class(const sf::Text& text) noexcept {
        return {font_name_map[&text.getFont()],
                text.getString(),
                text.getCharacterSize(),
                text.getLetterSpacing(),
                text.getLineSpacing(),
                text.getStyle(),
                text.getFillColor(),
                text.getOutlineColor(),
                text.getOutlineThickness()};
    }

    sf::Text to_class() const noexcept {
        sf::Text ret(font_map[font], text, size);
        ret.setLetterSpacing(letterSpacing);
        ret.setLineSpacing(lineSpacing);
        ret.setStyle(style);
        ret.setFillColor(fillColor);
        ret.setOutlineColor(outlineColor);
        ret.setOutlineThickness(outlineThickness);
        return ret;
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::Text, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          sf::Text, SfmlTextHelper> {};
}
