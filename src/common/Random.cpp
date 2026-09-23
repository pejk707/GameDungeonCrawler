#include "common/Random.h"

#include <algorithm>

namespace ll {

Random::Random(std::uint32_t seed) : engine_(seed) {}

int Random::range(int lo, int hi) {
    if (hi <= lo) return lo;
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(engine_);
}

bool Random::chance(double p) {
    if (p <= 0.0) return false;
    if (p >= 1.0) return true;
    std::bernoulli_distribution dist(p);
    return dist(engine_);
}

int FixedRandom::range(int lo, int hi) {
    if (ranges_.empty()) return lo;
    const int v = ranges_.front();
    ranges_.pop_front();
    return std::clamp(v, lo, std::max(lo, hi));
}

bool FixedRandom::chance(double p) {
    if (p <= 0.0) return false;
    if (chances_.empty()) return p >= 1.0;
    const bool v = chances_.front();
    chances_.pop_front();
    return v;
}

}  // namespace ll
