#pragma once

#include "common/Enums.h"

namespace ll {

struct GameContext;
struct ItemDef;

// Производные характеристики игрока: не хранятся, а вычисляются по экипировке и улучшениям.
namespace stats {
int attack(const GameContext& ctx);
int defense(const GameContext& ctx);
int flarePower(const GameContext& ctx);
int emberCount(const GameContext& ctx);
const ItemDef* equipped(const GameContext& ctx, EquipSlot slot);
}  // namespace stats

}  // namespace ll
