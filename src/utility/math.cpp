#include "utility/math.hpp"

namespace math {

uint32_t xorshift(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

xorshift_t xorshift() {
    static xorshift_t seed = randf((float)std::numeric_limits<xorshift_t>::min(), (float)std::numeric_limits<xorshift_t>::max());
    return seed = xorshift(seed);
}

}
