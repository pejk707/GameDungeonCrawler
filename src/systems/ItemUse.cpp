#include "systems/ItemUse.h"

#include <algorithm>
#include <string>

#include "core/GameContext.h"
#include "core/Systems.h"
#include "systems/PlayerStats.h"

namespace ll::items {

void pickUp(const ItemId& id, GameContext& ctx) {
    const ItemDef& item = ctx.data.item(id);
    PlayerState& p = ctx.world.player;
    if (item.type == ItemType::Upgrade) {
        p.maxHp += item.effect.maxHp;
        p.hp = std::min(p.maxHp, p.hp + item.effect.heal);
        p.maxOil += item.effect.maxOil;
        if (item.effect.maxHp == 0) p.upgrades.insert(id);  // сердцевина просто растворяется
        ctx.say(MsgType::Heal, item.useMessage.empty() ? ctx.fmt("take.ok", {{"item", item.name}}) : item.useMessage);
        return;
    }
    p.inventory.add(id);
    ctx.sayFmt(MsgType::Item, "take.ok", {{"item", item.name}});
    if (item.type == ItemType::Note) ctx.sys.hints.trigger("notes", ctx);
    if (item.slot && !p.equipment.slot(*item.slot)) {
        ctx.sayFmt(MsgType::Hint, "hint.format", {{"text", ctx.fmt("take.can_equip", {{"item", item.name}})}});
    }
}

bool useConsumable(const ItemDef& item, GameContext& ctx) {
    PlayerState& p = ctx.world.player;
    const ItemEffect& e = item.effect;
    if (e.heal > 0 && p.hp >= p.maxHp && e.oil == 0) {
        ctx.sayKey(MsgType::System, "use.full_hp");
        return false;
    }
    if (e.oil > 0 && p.oil >= p.maxOil && e.heal == 0) {
        ctx.sayKey(MsgType::System, "use.oil_full");
        return false;
    }
    if (e.heal == 0 && e.oil == 0) {
        ctx.sayKey(MsgType::System, "use.nothing");
        return false;
    }
    if (!item.useMessage.empty()) ctx.say(MsgType::Text, item.useMessage);
    if (e.heal > 0) {
        const int before = p.hp;
        p.hp = std::min(p.maxHp, p.hp + e.heal);
        ctx.sayFmt(MsgType::Heal, "use.healed", {{"hp", std::to_string(p.hp - before)}});
    }
    if (e.oil > 0) {
        const int added = ctx.sys.light.addOil(e.oil, ctx);
        ctx.sayFmt(MsgType::Oil, "light.oil_added", {{"oil", std::to_string(added)}});
    }
    p.inventory.remove(item.id);
    return true;
}

bool equip(const ItemDef& item, GameContext& ctx) {
    if (!item.slot) {
        ctx.sayFmt(MsgType::System, "equip.cannot", {{"item", item.name}});
        return false;
    }
    auto& slot = ctx.world.player.equipment.slot(*item.slot);
    if (slot == item.id) {
        ctx.sayFmt(MsgType::System, "equip.already", {{"item", item.name}});
        return false;
    }
    slot = item.id;
    if (*item.slot == EquipSlot::Weapon) {
        ctx.sayFmt(MsgType::Item, "equip.weapon", {{"item", item.name}, {"atk", std::to_string(stats::attack(ctx))}});
    } else {
        ctx.sayFmt(MsgType::Item, "equip.armor", {{"item", item.name}, {"def", std::to_string(stats::defense(ctx))}});
    }
    return true;
}

void readNote(const ItemDef& item, GameContext& ctx) {
    ctx.say(MsgType::Title, "« " + item.name + " »");
    ctx.say(MsgType::Lore, ctx.data.texts.get(item.textFile));
    auto& notes = ctx.world.player.readNotes;
    if (notes.insert(item.id).second) {
        ctx.sayFmt(MsgType::System, "read.journal", {{"n", std::to_string(notes.size())},
                                                     {"total", std::to_string(ctx.data.config.totalNotes)}});
    }
}

}  // namespace ll::items
