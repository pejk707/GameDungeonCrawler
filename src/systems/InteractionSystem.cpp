#include "systems/InteractionSystem.h"

#include <algorithm>
#include <string>

#include "core/GameContext.h"
#include "core/Systems.h"
#include "systems/PlayerStats.h"

namespace ll {
namespace {

int oilNeeded(const std::vector<Effect>& effects) {
    int total = 0;
    for (const auto& e : effects) {
        if (const auto* s = std::get_if<SpendOil>(&e)) total += s->amount;
    }
    return total;
}

}  // namespace

bool InteractionSystem::evaluate(const Condition& condition, const GameContext& ctx) const {
    return std::visit(overloaded{
                          [&](const HasFlag& c) { return ctx.world.hasFlag(c.flag); },
                          [&](const NotFlag& c) { return !ctx.world.hasFlag(c.flag); },
                          [&](const HasItem& c) { return ctx.world.player.inventory.has(c.item); },
                          [&](const EmberCount& c) { return stats::emberCount(ctx) >= c.min; },
                      },
                      condition);
}

bool InteractionSystem::allMet(const std::vector<Condition>& conditions, const GameContext& ctx) const {
    for (const auto& c : conditions) {
        if (!evaluate(c, ctx)) return false;
    }
    return true;
}

void InteractionSystem::apply(const Effect& effect, GameContext& ctx) const {
    std::visit(overloaded{
                   [&](const SetFlag& e) { ctx.world.setFlag(e.flag); },
                   [&](const ClearFlags& e) {
                       for (const auto& f : e.flags) ctx.world.clearFlag(f);
                   },
                   [&](const GiveItem& e) {
                       ctx.world.player.inventory.add(e.item, e.count);
                       ctx.sayFmt(MsgType::Item, "item.received", {{"item", ctx.data.item(e.item).name}});
                   },
                   [&](const ConsumeItem& e) { ctx.world.player.inventory.remove(e.item, e.count); },
                   [&](const RevealExit& e) { ctx.world.room(e.room).revealedExits.insert(e.dir); },
                   [&](const UnlockExit& e) { ctx.world.room(e.room).unlockedExits.insert(e.dir); },
                   [&](const SpawnEnemy& e) { ctx.sys.combat.spawnAndBegin(e.enemy, ctx); },
                   [&](const SpendOil& e) { ctx.sys.light.spendOil(e.amount, ctx); },
                   [&](const AddOil& e) {
                       const int added = ctx.sys.light.addOil(e.amount, ctx);
                       ctx.sayFmt(MsgType::Oil, "light.oil_added", {{"oil", std::to_string(added)}});
                   },
                   [&](const LightRoom&) {
                       ctx.world.currentRoom().lit = true;
                       ctx.world.player.darknessCounter = 0;
                   },
                   [&](const ShowText& e) { ctx.say(MsgType::Text, e.text); },
                   [&](const WinGame&) { ctx.request(StateId::Victory); },
               },
               effect);
}

InteractionOutcome InteractionSystem::run(Trigger trigger, const std::optional<ItemId>& item,
                                          const ObjectId& object, GameContext& ctx) const {
    const ObjectDef* def = ctx.data.room(ctx.world.player.location).findObject(object);
    if (!def) return InteractionOutcome::NotApplicable;
    ObjectState& os = ctx.world.currentRoom().objects[object];

    int index = -1;
    for (std::size_t i = 0; i < def->interactions.size(); ++i) {
        const InteractionDef& in = def->interactions[i];
        if (in.trigger != trigger) continue;
        if (trigger == Trigger::Use && in.useItem != item) continue;
        if (in.once && os.doneInteractions.count(static_cast<int>(i))) continue;
        index = static_cast<int>(i);
        break;
    }
    if (index < 0) return InteractionOutcome::NotApplicable;
    const InteractionDef& in = def->interactions[static_cast<std::size_t>(index)];

    // Автоматические требования: не путать штраф загадки с простой нехваткой ресурса.
    if (os.usesLeft == 0) {
        ctx.sayFmt(MsgType::System, "interaction.empty", {{"object", def->name}});
        return InteractionOutcome::Refused;
    }
    if (trigger == Trigger::Light && !ctx.sys.light.lanternBurning(ctx)) {
        ctx.sayKey(MsgType::System, "light.need_lantern");
        return InteractionOutcome::Refused;
    }
    const int oil = std::max(oilNeeded(in.effects), oilNeeded(in.onFail));
    if (oil > 0 && ctx.world.player.oil < oil) {
        ctx.sayFmt(MsgType::System, "light.need_oil", {{"oil", std::to_string(oil)}});
        return InteractionOutcome::Refused;
    }

    if (allMet(in.conditions, ctx)) {
        if (!in.message.empty()) ctx.say(MsgType::Text, in.message);
        for (const auto& e : in.effects) apply(e, ctx);
        if (in.once) os.doneInteractions.insert(index);
        if (os.usesLeft > 0) --os.usesLeft;
        return InteractionOutcome::Success;
    }
    if (!in.failMessage.empty()) ctx.say(MsgType::Damage, in.failMessage);
    for (const auto& e : in.onFail) apply(e, ctx);
    return InteractionOutcome::Failed;
}

InteractionOutcome InteractionSystem::use(const std::optional<ItemId>& item, const ObjectId& object,
                                          GameContext& ctx) const {
    return run(Trigger::Use, item, object, ctx);
}

InteractionOutcome InteractionSystem::lightBrazier(const ObjectId& object, GameContext& ctx) const {
    const ObjectDef* def = ctx.data.room(ctx.world.player.location).findObject(object);
    RoomState& room = ctx.world.currentRoom();
    if (room.lit) {
        ctx.sayFmt(MsgType::System, "brazier.already_lit", {{"object", def->name}});
        return InteractionOutcome::NotApplicable;
    }
    if (!ctx.sys.light.lanternBurning(ctx)) {
        ctx.sayKey(MsgType::System, "light.need_lantern");
        return InteractionOutcome::Refused;
    }
    const int cost = ctx.data.config.oilCost.brazier;
    if (ctx.world.player.oil < cost) {
        ctx.sayFmt(MsgType::System, "light.need_oil", {{"oil", std::to_string(cost)}});
        return InteractionOutcome::Refused;
    }
    ctx.sys.light.spendOil(cost, ctx);
    room.lit = true;
    ctx.world.player.darknessCounter = 0;
    ctx.sayFmt(MsgType::Oil, "brazier.lit", {{"object", def->name}, {"oil", std::to_string(cost)}});
    ctx.sys.hints.trigger("brazier_lit", ctx);
    return InteractionOutcome::Success;
}

InteractionOutcome InteractionSystem::light(const ObjectId& object, GameContext& ctx) const {
    const ObjectDef* def = ctx.data.room(ctx.world.player.location).findObject(object);
    if (!def) return InteractionOutcome::NotApplicable;
    if (def->kind == ObjectKind::Brazier) return lightBrazier(object, ctx);
    const auto outcome = run(Trigger::Light, std::nullopt, object, ctx);
    if (outcome == InteractionOutcome::NotApplicable) {
        ctx.sayFmt(MsgType::System, "interaction.cannot_light", {{"object", def->name}});
    }
    return outcome;
}

void InteractionSystem::listContents(const ObjectId& object, GameContext& ctx) const {
    const ObjectDef* def = ctx.data.room(ctx.world.player.location).findObject(object);
    const ObjectState& os = ctx.world.currentRoom().objects[object];
    if (os.contents.empty()) {
        ctx.sayFmt(MsgType::System, "container.empty", {{"object", def->name}});
        return;
    }
    std::string list;
    for (const auto& id : os.contents) {
        if (!list.empty()) list += ", ";
        list += ctx.data.item(id).name;
    }
    ctx.sayFmt(MsgType::Item, "container.contents", {{"object", def->name}, {"items", list}});
}

InteractionOutcome InteractionSystem::open(const ObjectId& object, GameContext& ctx) const {
    const ObjectDef* def = ctx.data.room(ctx.world.player.location).findObject(object);
    if (!def) return InteractionOutcome::NotApplicable;
    if (def->kind == ObjectKind::Container) {
        ObjectState& os = ctx.world.currentRoom().objects[object];
        if (!os.opened) {
            os.opened = true;
            ctx.sayFmt(MsgType::Text, "container.open", {{"object", def->name}});
        }
        listContents(object, ctx);
        return InteractionOutcome::Success;
    }
    const auto outcome = run(Trigger::Open, std::nullopt, object, ctx);
    if (outcome == InteractionOutcome::NotApplicable) {
        ctx.sayFmt(MsgType::System, "interaction.cannot_open", {{"object", def->name}});
    }
    return outcome;
}

void InteractionSystem::examine(const ObjectId& object, GameContext& ctx) const {
    const ObjectDef* def = ctx.data.room(ctx.world.player.location).findObject(object);
    if (!def) return;
    ctx.say(MsgType::Text, def->description);
    const ObjectState& os = ctx.world.currentRoom().objects[object];
    if (def->kind == ObjectKind::Container && os.opened) listContents(object, ctx);
    if (def->kind == ObjectKind::Readable) ctx.sayFmt(MsgType::Hint, "hint.readable", {{"object", def->name}});
    if (def->kind == ObjectKind::Brazier) {
        ctx.sayKey(MsgType::Oil, ctx.world.currentRoom().lit ? "brazier.burning" : "brazier.cold");
    }
    run(Trigger::Examine, std::nullopt, object, ctx);
}

}  // namespace ll
