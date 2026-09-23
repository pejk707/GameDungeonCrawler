#include "systems/DamageCalculator.h"

#include <algorithm>
#include <cmath>

namespace ll {

HitResult DamageCalculator::weaponHit(const WeaponHitInput& in, IRandom& rng) const {
    HitResult r;
    if (in.inDarkness && rng.chance(cfg_.darkMissChance)) {
        r.darkMiss = true;
        return r;
    }
    if (in.dodgeChance > 0.0 && rng.chance(in.dodgeChance)) {
        r.dodged = true;
        return r;
    }
    const int raw = in.attack + rng.range(0, 2);
    double dmg = std::max(1, raw - (in.overheated ? 0 : in.targetDef));
    if (in.guarding) dmg = std::ceil(dmg * cfg_.guardMult);
    r.crit = rng.chance(cfg_.critChance);
    if (r.crit) dmg *= cfg_.critMult;
    if (in.counter) dmg *= cfg_.counterMult;
    if (in.doubleDamage) dmg *= 2.0;
    r.hit = true;
    r.damage = std::max(1, static_cast<int>(dmg));
    return r;
}

int DamageCalculator::flare(int power, bool photophobic, bool doubleDamage) const {
    double dmg = power;
    if (photophobic) dmg *= cfg_.photophobiaMult;
    if (doubleDamage) dmg *= 2.0;
    return static_cast<int>(dmg);
}

int DamageCalculator::enemyHit(int atk, int playerDef, IRandom& rng) const {
    return std::max(1, atk + rng.range(0, 1) - playerDef);
}

int DamageCalculator::blocked(int damage, IntentType intent) const {
    if (intent == IntentType::Heavy) return 0;
    return static_cast<int>(std::floor(damage * cfg_.blockMult));
}

}  // namespace ll
