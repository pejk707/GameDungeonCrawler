#pragma once

#include "model/GameData.h"
#include "model/WorldState.h"

namespace ll {

// Строит начальное состояние мира по определениям (новая игра).
class WorldFactory {
public:
    static WorldState create(const GameData& data);
};

}  // namespace ll
