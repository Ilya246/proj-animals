#include <rfl.hpp>
#include <SFML/System.hpp>

struct Vector2fHelper {
    float x;
    float y;
    
    static Vector2fHelper from_class(const sf::Vector2f& v) noexcept {
        return Vector2fHelper{v.x, v.y};
    }
    
    sf::Vector2f to_class() const noexcept {
        return sf::Vector2f(x, y);
    }
};

namespace rfl::parsing {
template <class ReaderType, class WriterType, class ProcessorsType>
struct Parser<ReaderType, WriterType, sf::Vector2f, ProcessorsType>
    : public CustomParser<ReaderType, WriterType, ProcessorsType, 
                          sf::Vector2f, Vector2fHelper> {};
}
