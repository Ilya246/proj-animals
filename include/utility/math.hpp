#pragma once
#include <limits>
#include <random>
#include <sys/types.h>

namespace math {

typedef uint32_t xorshift_t;

// Generate random real in [min, max)
template<typename NumT>
inline NumT randf(NumT min, NumT max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<NumT> dis(min, max);
    return dis(gen);
}

inline uint32_t xorshift(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}
inline xorshift_t xorshift() {
    static xorshift_t seed = randf((float)std::numeric_limits<xorshift_t>::min(), (float)std::numeric_limits<xorshift_t>::max());
    return seed = xorshift(seed);
}
inline const xorshift_t xorshift_range = std::numeric_limits<xorshift_t>::max() - std::numeric_limits<xorshift_t>::min();
inline const double inv_xorshift_range = 1.0 / xorshift_range;

// Generate random real in [min, max]
template<typename NumT>
inline NumT rand(NumT min, NumT max) {
    return xorshift() * inv_xorshift_range * (max - min) + min;
}

}