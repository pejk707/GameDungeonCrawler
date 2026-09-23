#pragma once

#include <cstddef>
#include <vector>

#include "common/Enums.h"
#include "common/Ids.h"

namespace ll {

struct GameContext;
struct EnemyDef;
struct EnemyInstance;

struct CombatAction {
    enum class Kind { Attack, Block, Flare, UseItem, Flee };
    Kind kind = Kind::Attack;
    ItemId item;  // для UseItem
};

enum class RoundOutcome { Continue, EnemyDefeated, PlayerDefeated, Fled, NoTurn };

// Пошаговый бой один на один: враг заранее объявляет намерение, игрок отвечает.
class CombatSystem {
public:
    void begin(std::size_t enemyIndex, bool sneak, GameContext& ctx) const;
    void spawnAndBegin(const EnemyId& enemy, GameContext& ctx) const;
    RoundOutcome playerAction(const CombatAction& action, GameContext& ctx) const;

    void showHeader(GameContext& ctx) const;
    void announceIntent(GameContext& ctx) const;
    void showStatus(GameContext& ctx) const;
    void examineEnemy(GameContext& ctx) const;

    EnemyInstance* currentEnemy(GameContext& ctx) const;
    static const std::vector<IntentType>& pattern(const EnemyInstance& e, const EnemyDef& def);

private:
    void enemyActs(EnemyInstance& e, const EnemyDef& def, bool blocking, GameContext& ctx) const;
    void damageEnemy(EnemyInstance& e, const EnemyDef& def, int dmg, GameContext& ctx) const;
    void checkPhase(EnemyInstance& e, const EnemyDef& def, GameContext& ctx) const;
    void advanceIntent(EnemyInstance& e, const EnemyDef& def, GameContext& ctx) const;
    void defeat(GameContext& ctx, bool fromCombatState) const;
    void endEncounter(GameContext& ctx) const;
};

}  // namespace ll
