#pragma once

#include "common/Enums.h"
#include "common/Random.h"
#include "model/Defs.h"

namespace ll {

struct WeaponHitInput {
    int attack = 0;
    int targetDef = 0;
    bool guarding = false;      // враг объявил ЗАЩИТУ: урон ×0.5
    bool overheated = false;    // враг объявил ПЕРЕГРЕВ: его защита 0
    bool counter = false;       // контрудар после блока сильного удара: ×1.5
    bool doubleDamage = false;  // «Раскрыт» или внезапная атака: ×2
    bool inDarkness = false;    // фонарь не горит: шанс промаха
    double dodgeChance = 0.0;
};

struct HitResult {
    bool hit = false;
    bool crit = false;
    bool dodged = false;
    bool darkMiss = false;
    int damage = 0;
};

// Формулы боя из GDD (раздел 8.1). Чистые функции — легко тестировать с FixedRandom.
class DamageCalculator {
public:
    explicit DamageCalculator(const CombatConfig& cfg) : cfg_(cfg) {}

    HitResult weaponHit(const WeaponHitInput& in, IRandom& rng) const;
    int flare(int power, bool photophobic, bool doubleDamage) const;
    int enemyHit(int atk, int playerDef, IRandom& rng) const;
    int blocked(int damage, IntentType intent) const;  // удар ×0.25, сильный удар — 0

private:
    const CombatConfig& cfg_;
};

}  // namespace ll
