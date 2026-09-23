#pragma once

#include "common/Ids.h"

namespace ll {

struct GameContext;

// Контекстные подсказки: каждая показывается один раз (текст — strings.json, раздел hint).
class HintSystem {
public:
    void trigger(const HintId& id, GameContext& ctx) const;
    void onTurn(GameContext& ctx) const;
};

}  // namespace ll
