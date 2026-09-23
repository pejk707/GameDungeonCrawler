#include "ui/RoomDescriber.h"

#include <algorithm>
#include <utility>

#include "common/Utf8.h"
#include "core/GameContext.h"
#include "core/Systems.h"

namespace ll {

std::string RoomDescriber::bar(int value, int max, int cells) {
    if (max <= 0) max = 1;
    const int filled = std::clamp((value * cells + max / 2) / max, 0, cells);
    return utf8::repeat("█", static_cast<std::size_t>(filled)) +
           utf8::repeat("░", static_cast<std::size_t>(cells - filled));
}

std::string RoomDescriber::header(const std::string& left, const std::string& right, int width) {
    const std::string l = "── " + left + " ";
    const std::string r = right.empty() ? "──" : " " + right + " ──";
    const int used = static_cast<int>(utf8::length(l) + utf8::length(r));
    const int fill = std::max(2, width - used);
    return l + utf8::repeat("─", static_cast<std::size_t>(fill)) + r;
}

std::string RoomDescriber::statusLine(const GameContext& ctx) {
    const PlayerState& p = ctx.world.player;
    std::string line = ctx.fmt("status.line", {{"bar", bar(p.oil, p.maxOil, 10)},
                                               {"oil", std::to_string(p.oil)},
                                               {"max_oil", std::to_string(p.maxOil)},
                                               {"hp", std::to_string(p.hp)},
                                               {"max_hp", std::to_string(p.maxHp)},
                                               {"turn", std::to_string(ctx.world.stats.turns)}});
    if (!p.lanternLit) line += ctx.str("status.lantern_off");
    return line;
}

std::string RoomDescriber::itemList(const std::vector<ItemId>& items, const GameContext& ctx) {
    std::vector<std::pair<ItemId, int>> grouped;
    for (const auto& id : items) {
        auto it = std::find_if(grouped.begin(), grouped.end(), [&](const auto& g) { return g.first == id; });
        if (it == grouped.end()) {
            grouped.emplace_back(id, 1);
        } else {
            ++it->second;
        }
    }
    std::string out;
    for (const auto& [id, n] : grouped) {
        if (!out.empty()) out += ", ";
        out += ctx.data.item(id).name;
        if (n > 1) out += " ×" + std::to_string(n);
    }
    return out;
}

std::string RoomDescriber::exitsLine(const GameContext& ctx) {
    const RoomId& loc = ctx.world.player.location;
    std::string list;
    for (Direction d : kAllDirections) {
        const ExitDef* exit = ctx.sys.movement.findExit(loc, d, ctx);
        if (!exit) continue;
        if (!list.empty()) list += ", ";
        list += ctx.str("dir." + std::string(toKey(d)));
        if (ctx.sys.movement.isLocked(loc, *exit, ctx)) list += ctx.str("room.locked_suffix");
    }
    if (list.empty()) return ctx.str("room.no_exits");
    return ctx.fmt("room.exits", {{"exits", list}});
}

void RoomDescriber::describe(GameContext& ctx, bool brief, bool banner) {
    const RoomDef& def = ctx.data.room(ctx.world.player.location);
    const RoomState& room = ctx.world.currentRoom();
    const ZoneDef* zone = ctx.data.findZone(def.zone);
    const bool see = ctx.sys.light.canSee(ctx);
    const int width = ctx.data.config.width;

    if (banner && zone && ctx.data.texts.has(zone->banner)) {
        ctx.say(MsgType::Art, ctx.data.texts.get(zone->banner));
    }
    if (see) {
        ctx.say(MsgType::Title, header(def.name, zone ? zone->name : std::string{}, width));
    } else {
        const int threshold = ctx.data.config.darknessTurnsToDeath;
        const int counter = ctx.world.player.darknessCounter;
        std::string meter = utf8::repeat("■", static_cast<std::size_t>(std::min(counter, threshold))) +
                            utf8::repeat("□", static_cast<std::size_t>(std::max(0, threshold - counter)));
        ctx.say(MsgType::Title, header("???", ctx.fmt("room.darkness_meter", {{"meter", meter}}), width));
    }
    ctx.say(MsgType::Status, statusLine(ctx));

    if (!see) {
        ctx.say(MsgType::Dark, def.descriptionDark.empty() ? ctx.str("room.dark_default") : def.descriptionDark);
        for (const auto& e : room.enemies) {
            const EnemyDef& ed = ctx.data.enemy(e.def);
            if (!ed.sleepText.empty() && e.behavior == Behavior::Sleeping) ctx.say(MsgType::Dark, ed.sleepText);
        }
        return;
    }
    if (!brief) ctx.say(MsgType::Text, def.description);
    if (!room.items.empty()) {
        ctx.sayFmt(MsgType::Item, "room.items", {{"items", itemList(room.items, ctx)}});
    }
    for (const auto& e : room.enemies) {
        const std::string name = ctx.data.enemy(e.def).name;
        ctx.sayFmt(MsgType::Damage, e.behavior == Behavior::Sleeping ? "room.enemy_sleeping" : "room.enemy_awake",
                   {{"enemy", name}});
    }
    ctx.say(MsgType::System, exitsLine(ctx));
}

}  // namespace ll
