#include "systems/PlayerStats.h"

#include "core/GameContext.h"

namespace ll::stats {

const ItemDef* equipped(const GameContext& ctx, EquipSlot slot) {
    const auto& id = ctx.world.player.equipment.slot(slot);
    return id ? ctx.data.findItem(*id) : nullptr;
}

int attack(const GameContext& ctx) {
    const ItemDef* weapon = equipped(ctx, EquipSlot::Weapon);
    return ctx.world.player.baseAtk + (weapon ? weapon->atkBonus : 0);
}

int defense(const GameContext& ctx) {
    int d = 0;
    for (EquipSlot s : {EquipSlot::Body, EquipSlot::Head}) {
        if (const ItemDef* item = equipped(ctx, s)) d += item->defBonus;
    }
    return d;
}

int flarePower(const GameContext& ctx) {
    int power = ctx.data.config.combat.flareDamage;
    for (const auto& id : ctx.world.player.upgrades) {
        if (const ItemDef* item = ctx.data.findItem(id)) power += item->flareBonus;
    }
    if (const ItemDef* weapon = equipped(ctx, EquipSlot::Weapon)) power += weapon->flareBonus;
    return power;
}

int emberCount(const GameContext& ctx) {
    int n = 0;
    for (const auto& id : ctx.data.config.embers) {
        if (ctx.world.player.inventory.has(id)) ++n;
    }
    return n;
}

}  // namespace ll::stats
