#pragma once

#include "common/Ids.h"

namespace ll {

struct GameContext;
struct ItemDef;

// Действия с предметами, общие для исследования и боя.
namespace items {

// Подобрать предмет: улучшения применяются сразу, остальное — в инвентарь.
void pickUp(const ItemId& id, GameContext& ctx);

// Применить расходник (лечение, масло). false — предмет нельзя или бессмысленно применять.
bool useConsumable(const ItemDef& item, GameContext& ctx);

// Надеть оружие или броню.
bool equip(const ItemDef& item, GameContext& ctx);

// Прочитать записку и отметить её в журнале.
void readNote(const ItemDef& item, GameContext& ctx);

}  // namespace items
}  // namespace ll
