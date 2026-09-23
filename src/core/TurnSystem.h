#pragma once

namespace ll {

struct GameContext;

// Всё, что происходит в конце хода: счётчик ходов, тьма, спящие враги, подсказки.
class TurnSystem {
public:
    void endTurn(GameContext& ctx) const;
    void endCombatRound(GameContext& ctx) const;  // в бою тьма не убивает
};

}  // namespace ll
