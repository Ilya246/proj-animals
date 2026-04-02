#include "graphics/texture.hpp"
#include <rfl.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

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
    const sf::Texture& tex;

    static SfmlSpriteHelper from_class(const sf::Sprite& sprite) noexcept {
        return SfmlSpriteHelper{
            sprite.getTexture()
        };
    }

    sf::Sprite to_class() const noexcept {
        return sf::Sprite(tex);
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::Sprite, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          sf::Sprite, SfmlSpriteHelper> {};
}