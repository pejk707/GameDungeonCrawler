#include <algorithm>
#include <string>

#include "commands/HandlerBase.h"
#include "common/Utf8.h"
#include "core/GameContext.h"
#include "core/Systems.h"
#include "systems/ItemUse.h"
#include "systems/PlayerStats.h"
#include "ui/RoomDescriber.h"

namespace ll {
namespace {

// ---------- перемещение и осмотр ----------

class GoHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        if (!cmd.direction) {
            ctx.sayKey(MsgType::System, "parse.no_direction");
            return ActionResult::noTurn();
        }
        const MoveResult r = ctx.sys.movement.move(*cmd.direction, ctx);
        if (r.moved) {
            RoomDescriber::describe(ctx, false, r.newZone);
            ctx.sys.movement.arrive(ctx);
        }
        return {r.tookTurn, false, false};
    }
};

class LookHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        RoomDescriber::describe(ctx);
        return ActionResult::noTurn();
    }
};

class ExamineHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        if (cmd.object.empty()) {
            RoomDescriber::describe(ctx);
            return ActionResult::noTurn();
        }
        ActionResult result;
        const auto ref = find(cmd.object, scope::All, ctx, result);
        if (!ref) return result;
        switch (ref->kind) {
            case EntityKind::Object:
                ctx.sys.interaction.examine(ref->id, ctx);
                break;
            case EntityKind::Enemy: {
                const EnemyDef& e = ctx.data.enemy(ref->id);
                const EnemyInstance& inst = ctx.world.currentRoom().enemies[ref->index];
                ctx.say(MsgType::Text, e.description);
                ctx.sayFmt(MsgType::Damage, "enemy.hp",
                           {{"enemy", e.name}, {"hp", std::to_string(inst.hp)}, {"max", std::to_string(e.hp)}});
                break;
            }
            default: {
                const ItemDef& item = ctx.data.item(ref->id);
                ctx.say(MsgType::Text, item.description);
                if (item.atkBonus) ctx.sayFmt(MsgType::Item, "item.atk", {{"n", std::to_string(item.atkBonus)}});
                if (item.defBonus) ctx.sayFmt(MsgType::Item, "item.def", {{"n", std::to_string(item.defBonus)}});
                if (item.type == ItemType::Note) ctx.sayFmt(MsgType::Hint, "hint.readable", {{"object", item.name}});
            }
        }
        return result;
    }
};

// ---------- предметы ----------

class TakeHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        if (!canSee(ctx)) {
            ctx.sayKey(MsgType::Dark, "take.too_dark");
            return ActionResult::noTurn();
        }
        RoomState& room = ctx.world.currentRoom();
        if (cmd.all && cmd.object.empty()) return takeAll(ctx, room);

        ActionResult result;
        const auto ref = find(cmd.object, scope::All, ctx, result);
        if (!ref) return result;
        switch (ref->kind) {
            case EntityKind::Inventory:
                ctx.sayFmt(MsgType::System, "take.already", {{"item", ctx.data.item(ref->id).name}});
                return result;
            case EntityKind::Object:
                ctx.sayFmt(MsgType::System, "take.cannot", {{"item", EntityResolver::name(*ref, ctx)}});
                return result;
            case EntityKind::Enemy:
                ctx.sayKey(MsgType::System, "take.enemy");
                return result;
            case EntityKind::Floor:
                room.items.erase(std::find(room.items.begin(), room.items.end(), ref->id));
                break;
            case EntityKind::Container: {
                auto& contents = room.objects[ref->container].contents;
                contents.erase(std::find(contents.begin(), contents.end(), ref->id));
                break;
            }
        }
        items::pickUp(ref->id, ctx);
        return ActionResult::turn();
    }

private:
    ActionResult takeAll(GameContext& ctx, RoomState& room) const {
        std::vector<ItemId> taken = room.items;
        room.items.clear();
        for (auto& [objId, os] : room.objects) {
            if (!os.opened) continue;
            taken.insert(taken.end(), os.contents.begin(), os.contents.end());
            os.contents.clear();
        }
        if (taken.empty()) {
            ctx.sayKey(MsgType::System, "take.nothing");
            return ActionResult::noTurn();
        }
        for (const auto& id : taken) items::pickUp(id, ctx);
        return ActionResult::turn();
    }
};

class DropHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        ActionResult result;
        const auto ref = find(cmd.object, scope::Inventory, ctx, result);
        if (!ref) return result;
        const ItemDef& item = ctx.data.item(ref->id);
        if (!item.droppable) {
            ctx.sayFmt(MsgType::System, "drop.cannot", {{"item", item.name}});
            return result;
        }
        PlayerState& p = ctx.world.player;
        p.inventory.remove(item.id);
        if (item.slot && p.equipment.slot(*item.slot) == item.id && !p.inventory.has(item.id)) {
            p.equipment.slot(*item.slot).reset();
        }
        ctx.world.currentRoom().items.push_back(item.id);
        ctx.sayFmt(MsgType::System, "drop.ok", {{"item", item.name}});
        return ActionResult::turn();
    }
};

class OpenHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        ActionResult result;
        const auto ref = find(cmd.object, scope::Objects | scope::Inventory | scope::Floor, ctx, result);
        if (!ref) return result;
        if (ref->kind != EntityKind::Object) {
            ctx.sayKey(MsgType::System, "open.item");
            return result;
        }
        const auto outcome = ctx.sys.interaction.open(ref->id, ctx);
        result.tookTurn = outcome == InteractionOutcome::Success || outcome == InteractionOutcome::Failed;
        return result;
    }
};

class UseHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        ActionResult result;
        const auto ref = find(cmd.object, scope::Inventory | scope::Objects | scope::Floor, ctx, result, false,
                              "use.what");
        if (!ref) return result;

        if (ref->kind == EntityKind::Object) {  // «использовать цистерну»
            return interact(std::nullopt, ref->id, ctx);
        }
        if (ref->kind == EntityKind::Floor) {
            ctx.sayFmt(MsgType::System, "use.take_first", {{"item", ctx.data.item(ref->id).name}});
            return result;
        }
        const ItemDef& item = ctx.data.item(ref->id);
        if (!cmd.target.empty()) {
            const auto target = find(cmd.target, scope::Objects, ctx, result, true);
            if (!target) return result;
            return interact(item.id, target->id, ctx);
        }
        switch (item.type) {
            case ItemType::Consumable:
                if (item.combatOnly) {
                    ctx.sayFmt(MsgType::System, "use.combat_only", {{"item", item.name}});
                    return result;
                }
                result.tookTurn = items::useConsumable(item, ctx);
                return result;
            case ItemType::Weapon:
            case ItemType::Armor:
                result.tookTurn = items::equip(item, ctx);
                return result;
            case ItemType::Note:
                items::readNote(item, ctx);
                return result;
            case ItemType::Lantern:
                ctx.sys.light.setLantern(!ctx.world.player.lanternLit, ctx);
                return result;
            default:
                ctx.sayFmt(MsgType::System, "use.on_what", {{"item", item.name}});
                return result;
        }
    }

private:
    static ActionResult interact(const std::optional<ItemId>& item, const ObjectId& object, GameContext& ctx) {
        const auto outcome = ctx.sys.interaction.use(item, object, ctx);
        if (outcome == InteractionOutcome::NotApplicable) ctx.sayKey(MsgType::System, "use.nothing_happens");
        const bool turn = outcome == InteractionOutcome::Success || outcome == InteractionOutcome::Failed;
        return {turn, false, false};
    }
};

class EquipHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        ActionResult result;
        const auto ref = find(cmd.object, scope::Inventory | scope::Floor, ctx, result);
        if (!ref) return result;
        if (ref->kind != EntityKind::Inventory) {
            ctx.sayFmt(MsgType::System, "use.take_first", {{"item", ctx.data.item(ref->id).name}});
            return result;
        }
        result.tookTurn = items::equip(ctx.data.item(ref->id), ctx);
        return result;
    }
};

class ReadHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        ActionResult result;
        if (!canSee(ctx)) {
            ctx.sayKey(MsgType::Dark, "read.too_dark");
            return result;
        }
        const auto ref = find(cmd.object, scope::Inventory | scope::Floor | scope::Containers | scope::Objects,
                              ctx, result);
        if (!ref) return result;
        if (ref->kind == EntityKind::Object) {
            const ObjectDef* o = ctx.data.room(ctx.world.player.location).findObject(ref->id);
            if (o && o->kind == ObjectKind::Readable) {
                ctx.say(MsgType::Lore, o->text);
            } else {
                ctx.sayKey(MsgType::System, "read.cannot");
            }
            return result;
        }
        const ItemDef& item = ctx.data.item(ref->id);
        if (item.type != ItemType::Note) {
            ctx.sayKey(MsgType::System, "read.cannot");
            return result;
        }
        items::readNote(item, ctx);
        return result;
    }
};

// ---------- свет ----------

class LanternHandler final : public HandlerBase {
public:
    explicit LanternHandler(std::optional<bool> mode) : mode_(mode) {}
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        const bool before = canSee(ctx);
        const bool on = mode_.value_or(!ctx.world.player.lanternLit);
        if (ctx.sys.light.setLantern(on, ctx) && canSee(ctx) != before) RoomDescriber::describe(ctx);
        return ActionResult::noTurn();
    }

private:
    std::optional<bool> mode_;  // std::nullopt — переключить
};

class LightHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        ActionResult result;
        const auto ref = find(cmd.object, scope::Inventory | scope::Objects, ctx, result, false, "light.what");
        if (!ref) return result;
        if (ref->kind != EntityKind::Object) {
            if (ctx.data.item(ref->id).type == ItemType::Lantern) {
                return LanternHandler(true).execute(cmd, ctx);
            }
            ctx.sayFmt(MsgType::System, "interaction.cannot_light", {{"object", EntityResolver::name(*ref, ctx)}});
            return result;
        }
        const auto outcome = ctx.sys.interaction.light(ref->id, ctx);
        result.tookTurn = outcome == InteractionOutcome::Success || outcome == InteractionOutcome::Failed;
        return result;
    }
};

class RestHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        const RoomDef& def = ctx.data.room(ctx.world.player.location);
        const RoomState& room = ctx.world.currentRoom();
        if (!def.brazier() || !room.lit) {
            ctx.sayKey(MsgType::System, "rest.no_fire");
            return ActionResult::noTurn();
        }
        if (!room.enemies.empty()) {
            ctx.sayKey(MsgType::Damage, "rest.enemies");
            return ActionResult::noTurn();
        }
        PlayerState& p = ctx.world.player;
        p.hp = p.maxHp;
        ctx.sayKey(MsgType::Heal, "rest.ok");
        return ActionResult::turn();
    }
};

// ---------- информация ----------

class InventoryHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        const PlayerState& p = ctx.world.player;
        ctx.sayKey(MsgType::Title, "inventory.title");
        for (const auto& [id, n] : p.inventory.entries()) {
            const ItemDef& item = ctx.data.item(id);
            std::string line = "  " + item.name;
            if (n > 1) line += " ×" + std::to_string(n);
            if (p.equipment.isEquipped(id)) line += ctx.str("inventory.equipped");
            ctx.say(MsgType::Item, line);
        }
        for (const auto& id : p.upgrades) {
            ctx.say(MsgType::Oil, "  " + ctx.data.item(id).name + ctx.str("inventory.installed"));
        }
        ctx.sayFmt(MsgType::Status, "inventory.stats", {{"atk", std::to_string(stats::attack(ctx))},
                                                        {"def", std::to_string(stats::defense(ctx))},
                                                        {"flare", std::to_string(stats::flarePower(ctx))}});
        return ActionResult::noTurn();
    }
};

class StatusHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        ctx.say(MsgType::Status, RoomDescriber::statusLine(ctx));
        ctx.sayFmt(MsgType::Status, "status.details",
                   {{"atk", std::to_string(stats::attack(ctx))},
                    {"def", std::to_string(stats::defense(ctx))},
                    {"flare", std::to_string(stats::flarePower(ctx))},
                    {"embers", std::to_string(stats::emberCount(ctx))},
                    {"notes", std::to_string(ctx.world.player.readNotes.size())},
                    {"total", std::to_string(ctx.data.config.totalNotes)}});
        return ActionResult::noTurn();
    }
};

class JournalHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        const auto& notes = ctx.world.player.readNotes;
        if (notes.empty()) {
            ctx.sayKey(MsgType::System, "journal.empty");
            return ActionResult::noTurn();
        }
        ctx.sayFmt(MsgType::Title, "journal.title", {{"n", std::to_string(notes.size())},
                                                     {"total", std::to_string(ctx.data.config.totalNotes)}});
        for (const auto& id : notes) ctx.say(MsgType::Lore, "  " + ctx.data.item(id).name);
        ctx.sayKey(MsgType::Hint, "journal.hint");
        return ActionResult::noTurn();
    }
};

class HelpHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        ctx.sayKey(MsgType::Title, "help.title");
        ctx.sayKey(MsgType::Text, "help.exploration");
        return ActionResult::noTurn();
    }
};

class HintsHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) override {
        bool on = !ctx.settings.hints;
        for (const auto& w : cmd.object) {
            if (w == "вкл" || w == "on" || w == "да") on = true;
            if (w == "выкл" || w == "off" || w == "нет") on = false;
        }
        ctx.settings.hints = on;
        ctx.sayKey(MsgType::System, on ? "hints.on" : "hints.off");
        return ActionResult::noTurn();
    }
};

class QuitHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        ctx.sayKey(MsgType::System, "quit.to_menu");
        ctx.request(StateId::MainMenu);
        return ActionResult::noTurn();
    }
};

class EatHandler final : public HandlerBase {
public:
    ActionResult execute(const ParsedCommand&, GameContext& ctx) override {
        ctx.sayKey(MsgType::System, "eat.joke");
        return ActionResult::noTurn();
    }
};

}  // namespace

CommandRegistry makeExplorationCommands() {
    CommandRegistry r;
    r.add(Verb::Go, std::make_unique<GoHandler>());
    r.add(Verb::Look, std::make_unique<LookHandler>());
    r.add(Verb::Examine, std::make_unique<ExamineHandler>());
    r.add(Verb::Take, std::make_unique<TakeHandler>());
    r.add(Verb::Drop, std::make_unique<DropHandler>());
    r.add(Verb::Open, std::make_unique<OpenHandler>());
    r.add(Verb::Use, std::make_unique<UseHandler>());
    r.add(Verb::Equip, std::make_unique<EquipHandler>());
    r.add(Verb::Read, std::make_unique<ReadHandler>());
    r.add(Verb::Lantern, std::make_unique<LanternHandler>(std::nullopt));
    r.add(Verb::LanternOn, std::make_unique<LanternHandler>(true));
    r.add(Verb::LanternOff, std::make_unique<LanternHandler>(false));
    r.add(Verb::Light, std::make_unique<LightHandler>());
    r.add(Verb::Rest, std::make_unique<RestHandler>());
    r.add(Verb::Inventory, std::make_unique<InventoryHandler>());
    r.add(Verb::Status, std::make_unique<StatusHandler>());
    r.add(Verb::Journal, std::make_unique<JournalHandler>());
    r.add(Verb::Help, std::make_unique<HelpHandler>());
    r.add(Verb::Hints, std::make_unique<HintsHandler>());
    r.add(Verb::Quit, std::make_unique<QuitHandler>());
    r.add(Verb::Eat, std::make_unique<EatHandler>());
    return r;
}

}  // namespace ll
