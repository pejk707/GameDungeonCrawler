#include "systems/CombatSystem.h"

#include <algorithm>
#include <string>

#include "common/Utf8.h"
#include "core/GameContext.h"
#include "core/Systems.h"
#include "systems/DamageCalculator.h"
#include "systems/ItemUse.h"
#include "systems/PlayerStats.h"
#include "ui/RoomDescriber.h"

namespace ll {
namespace {

std::string num(int n) { return std::to_string(n); }

std::string intentLabel(IntentType intent, const EnemyDef& def, const GameContext& ctx) {
    const std::string key = "intent." + std::string(toKey(intent));
    switch (intent) {
        case IntentType::Snuff: return ctx.fmt(key, {{"oil", num(def.snuffAmount)}});
        case IntentType::Drain: return ctx.fmt(key, {{"oil", num(def.drainAmount)}});
        default: return ctx.str(key);
    }
}

}  // namespace

const std::vector<IntentType>& CombatSystem::pattern(const EnemyInstance& e, const EnemyDef& def) {
    if (e.phase > 0 && static_cast<std::size_t>(e.phase) <= def.phases.size()) {
        return def.phases[static_cast<std::size_t>(e.phase) - 1].pattern;
    }
    return def.pattern;
}

EnemyInstance* CombatSystem::currentEnemy(GameContext& ctx) const {
    if (!ctx.world.encounter) return nullptr;
    auto& enemies = ctx.world.room(ctx.world.encounter->room).enemies;
    const std::size_t i = ctx.world.encounter->enemyIndex;
    return i < enemies.size() ? &enemies[i] : nullptr;
}

void CombatSystem::begin(std::size_t enemyIndex, bool sneak, GameContext& ctx) const {
    RoomState& room = ctx.world.currentRoom();
    if (enemyIndex >= room.enemies.size()) return;
    EnemyInstance& e = room.enemies[enemyIndex];
    const EnemyDef& def = ctx.data.enemy(e.def);
    e.behavior = Behavior::Aggressive;
    e.wakeIn = -1;

    if (!e.engaged) {
        e.engaged = true;
        const int n = static_cast<int>(pattern(e, def).size());
        e.patternIndex = def.has(Trait::Boss) ? 0 : ctx.rng.range(0, n - 1);
        if (!sneak && !def.introText.empty()) ctx.say(MsgType::Damage, def.introText);
    } else if (!sneak) {
        ctx.sayFmt(MsgType::Damage, "combat.again", {{"enemy", def.name}});
    }

    Encounter enc;
    enc.room = ctx.world.player.location;
    enc.enemyIndex = enemyIndex;
    enc.intent = pattern(e, def)[static_cast<std::size_t>(e.patternIndex)];
    ctx.world.encounter = enc;
    ctx.lastCombatRoom = enc.room;

    if (sneak) {
        DamageCalculator calc(ctx.data.config.combat);
        WeaponHitInput in;
        in.attack = stats::attack(ctx);
        in.targetDef = def.def;
        in.doubleDamage = true;
        in.inDarkness = !ctx.sys.light.canSee(ctx);
        const HitResult hit = calc.weaponHit(in, ctx.rng);
        if (hit.hit) {
            ctx.sayFmt(MsgType::Damage, "combat.sneak", {{"enemy", def.name}});
            damageEnemy(e, def, hit.damage, ctx);
            if (e.hp <= 0) {
                defeat(ctx, false);
                return;
            }
        } else {
            ctx.sayKey(MsgType::System, "combat.miss_dark");
        }
    }
    ctx.request(StateId::Combat);
}

void CombatSystem::spawnAndBegin(const EnemyId& enemy, GameContext& ctx) const {
    EnemyInstance inst;
    inst.def = enemy;
    inst.hp = ctx.data.enemy(enemy).hp;
    ctx.world.currentRoom().enemies.push_back(inst);
    begin(ctx.world.currentRoom().enemies.size() - 1, false, ctx);
}

void CombatSystem::showHeader(GameContext& ctx) const {
    EnemyInstance* e = currentEnemy(ctx);
    if (!e) return;
    const EnemyDef& def = ctx.data.enemy(e->def);
    ctx.log.blank();
    ctx.say(MsgType::Title, RoomDescriber::header(ctx.fmt("combat.header", {{"enemy", utf8::toUpper(def.name)}}), "",
                                                  ctx.data.config.width));
    if (ctx.data.texts.has(def.art)) ctx.say(MsgType::Art, ctx.data.texts.get(def.art));
    ctx.sayFmt(MsgType::Damage, "combat.enemy_hp",
               {{"enemy", def.name}, {"bar", RoomDescriber::bar(e->hp, def.hp, 20)}, {"hp", num(e->hp)},
                {"max", num(def.hp)}});
}

void CombatSystem::announceIntent(GameContext& ctx) const {
    EnemyInstance* e = currentEnemy(ctx);
    if (!e) return;
    const EnemyDef& def = ctx.data.enemy(e->def);
    const IntentType intent = ctx.world.encounter->intent;
    auto it = def.intentText.find(intent);
    if (it != def.intentText.end()) ctx.say(MsgType::Text, it->second);
    ctx.sayFmt(MsgType::Intent, "combat.intent", {{"label", intentLabel(intent, def, ctx)}});
    ctx.sys.hints.trigger("intent_" + std::string(toKey(intent)), ctx);
    if (def.has(Trait::Photophobic)) ctx.sys.hints.trigger("photophobic", ctx);
    if (def.has(Trait::LightEater)) ctx.sys.hints.trigger("light_eater", ctx);
}

void CombatSystem::showStatus(GameContext& ctx) const {
    EnemyInstance* e = currentEnemy(ctx);
    if (!e) return;
    const EnemyDef& def = ctx.data.enemy(e->def);
    const PlayerState& p = ctx.world.player;
    ctx.sayFmt(MsgType::Status, "combat.status",
               {{"hp", num(p.hp)}, {"max_hp", num(p.maxHp)}, {"oil", num(p.oil)}, {"max_oil", num(p.maxOil)},
                {"enemy", def.name}, {"ehp", num(e->hp)}, {"emax", num(def.hp)}});
}

void CombatSystem::examineEnemy(GameContext& ctx) const {
    EnemyInstance* e = currentEnemy(ctx);
    if (!e) return;
    const EnemyDef& def = ctx.data.enemy(e->def);
    ctx.say(MsgType::Text, def.description);
    ctx.sayFmt(MsgType::Damage, "enemy.hp", {{"enemy", def.name}, {"hp", num(e->hp)}, {"max", num(def.hp)}});
    if (def.has(Trait::Photophobic)) ctx.sayKey(MsgType::Oil, "combat.trait_photophobic");
    if (def.has(Trait::LightEater)) ctx.sayKey(MsgType::Oil, "combat.trait_light_eater");
    if (def.dodgeChance > 0) ctx.sayKey(MsgType::System, "combat.trait_dodge");
    if (def.has(Trait::Boss)) ctx.sayKey(MsgType::System, "combat.trait_boss");
}

void CombatSystem::damageEnemy(EnemyInstance& e, const EnemyDef& def, int dmg, GameContext& ctx) const {
    e.hp = std::max(0, e.hp - dmg);
    ctx.sayFmt(MsgType::Heal, "combat.hit", {{"enemy", def.name}, {"dmg", num(dmg)}, {"hp", num(e.hp)},
                                             {"max", num(def.hp)}});
}

RoundOutcome CombatSystem::playerAction(const CombatAction& action, GameContext& ctx) const {
    EnemyInstance* ep = currentEnemy(ctx);
    if (!ep) return RoundOutcome::NoTurn;
    EnemyInstance& e = *ep;
    const EnemyDef& def = ctx.data.enemy(e.def);
    Encounter& enc = *ctx.world.encounter;
    const IntentType intent = enc.intent;
    const DamageCalculator calc(ctx.data.config.combat);
    const bool boss = def.has(Trait::Boss);
    bool blocking = false;
    bool stunned = false;

    switch (action.kind) {
        case CombatAction::Kind::Attack: {
            WeaponHitInput in;
            in.attack = stats::attack(ctx);
            in.targetDef = def.def;
            in.guarding = intent == IntentType::Guard;
            in.overheated = intent == IntentType::Overheat;
            in.counter = enc.counterReady;
            in.doubleDamage = e.exposed;
            in.inDarkness = !ctx.sys.light.canSee(ctx);
            in.dodgeChance = def.dodgeChance;
            enc.counterReady = false;
            const HitResult hit = calc.weaponHit(in, ctx.rng);
            if (hit.darkMiss) {
                ctx.sayKey(MsgType::System, "combat.miss_dark");
            } else if (hit.dodged) {
                ctx.sayFmt(MsgType::System, "combat.dodge", {{"enemy", def.name}});
            } else {
                if (in.counter) ctx.sayKey(MsgType::Heal, "combat.counter_hit");
                if (in.doubleDamage) ctx.sayKey(MsgType::Heal, "combat.exposed_hit");
                if (in.overheated) ctx.sayKey(MsgType::Heal, "combat.overheat_hit");
                if (in.guarding) ctx.sayFmt(MsgType::System, "combat.guarded", {{"enemy", def.name}});
                if (hit.crit) ctx.sayKey(MsgType::Heal, "combat.crit");
                damageEnemy(e, def, hit.damage, ctx);
            }
            break;
        }
        case CombatAction::Kind::Block:
            blocking = true;
            ctx.sayKey(MsgType::System, "combat.block");
            break;
        case CombatAction::Kind::Flare: {
            const int cost = ctx.data.config.oilCost.flare;
            if (!ctx.sys.light.lanternBurning(ctx) || ctx.world.player.oil < cost) {
                ctx.sayFmt(MsgType::System, "combat.no_flare", {{"oil", num(cost)}});
                return RoundOutcome::NoTurn;
            }
            ctx.sys.light.spendOil(cost, ctx);
            const int power = stats::flarePower(ctx);
            if (def.has(Trait::LightEater) && !e.exposed) {
                e.hp = std::min(def.hp, e.hp + power);
                ctx.sayFmt(MsgType::Damage, "combat.flare_feeds", {{"enemy", def.name}, {"heal", num(power)}});
                break;
            }
            const bool photophobic = def.has(Trait::Photophobic);
            ctx.sayKey(MsgType::Oil, "combat.flare");
            damageEnemy(e, def, calc.flare(power, photophobic, e.exposed), ctx);
            if (photophobic && !boss && e.hp > 0) {
                stunned = true;
                ctx.sayFmt(MsgType::Heal, "combat.stunned", {{"enemy", def.name}});
            }
            break;
        }
        case CombatAction::Kind::UseItem: {
            const ItemDef* item = ctx.data.findItem(action.item);
            if (!item || !ctx.world.player.inventory.has(action.item) || item->type != ItemType::Consumable) {
                ctx.sayKey(MsgType::System, "combat.cant_use");
                return RoundOutcome::NoTurn;
            }
            if (item->effect.escape) {
                if (boss) {
                    ctx.sayKey(MsgType::System, "combat.no_escape");
                    return RoundOutcome::NoTurn;
                }
                ctx.world.player.inventory.remove(item->id);
                ctx.say(MsgType::Text, item->useMessage);
                endEncounter(ctx);
                ctx.sys.movement.retreat(ctx);
                ctx.sayKey(MsgType::System, "combat.fled");
                ctx.request(StateId::Exploration);
                return RoundOutcome::Fled;
            }
            if (item->effect.damage > 0) {
                ctx.world.player.inventory.remove(item->id);
                ctx.say(MsgType::Oil, item->useMessage);
                damageEnemy(e, def, item->effect.damage, ctx);
            } else if (!items::useConsumable(*item, ctx)) {
                return RoundOutcome::NoTurn;
            }
            break;
        }
        case CombatAction::Kind::Flee: {
            if (boss) {
                ctx.sayKey(MsgType::System, "combat.no_flee");
                return RoundOutcome::NoTurn;
            }
            if (ctx.rng.chance(ctx.data.config.combat.fleeChance)) {
                endEncounter(ctx);
                ctx.sys.movement.retreat(ctx);
                ctx.sayKey(MsgType::System, "combat.fled");
                ctx.request(StateId::Exploration);
                return RoundOutcome::Fled;
            }
            ctx.sayKey(MsgType::Damage, "combat.flee_failed");
            break;
        }
    }

    if (e.hp <= 0) {
        defeat(ctx, true);
        return RoundOutcome::EnemyDefeated;
    }
    if (!stunned) enemyActs(e, def, blocking, ctx);
    if (ctx.world.player.hp <= 0) {
        ctx.deathCause = ctx.fmt("death.combat", {{"enemy", def.name}});
        endEncounter(ctx);
        ctx.request(StateId::GameOver);
        return RoundOutcome::PlayerDefeated;
    }
    if (intent != IntentType::Drain) e.exposed = false;  // «Раскрыт» длится ровно один раунд
    checkPhase(e, def, ctx);
    advanceIntent(e, def, ctx);
    ++enc.round;
    ctx.sys.turn.endCombatRound(ctx);
    return RoundOutcome::Continue;
}

void CombatSystem::enemyActs(EnemyInstance& e, const EnemyDef& def, bool blocking, GameContext& ctx) const {
    const DamageCalculator calc(ctx.data.config.combat);
    PlayerState& p = ctx.world.player;
    const IntentType intent = ctx.world.encounter->intent;
    auto hurt = [&](int dmg) { p.hp = std::max(0, p.hp - dmg); };

    switch (intent) {
        case IntentType::Attack: {
            int dmg = calc.enemyHit(def.atk, stats::defense(ctx), ctx.rng);
            if (blocking) {
                dmg = calc.blocked(dmg, intent);
                ctx.sayFmt(MsgType::System, "combat.enemy_hit_blocked", {{"enemy", def.name}, {"dmg", num(dmg)}});
            } else {
                ctx.sayFmt(MsgType::Damage, "combat.enemy_hit", {{"enemy", def.name}, {"dmg", num(dmg)}});
            }
            hurt(dmg);
            break;
        }
        case IntentType::Heavy: {
            if (blocking) {
                ctx.world.encounter->counterReady = true;
                ctx.sayFmt(MsgType::Heal, "combat.heavy_blocked", {{"enemy", def.name}});
            } else {
                const int dmg = calc.enemyHit(def.atk, stats::defense(ctx), ctx.rng) * 2;
                ctx.sayFmt(MsgType::Damage, "combat.enemy_heavy", {{"enemy", def.name}, {"dmg", num(dmg)}});
                hurt(dmg);
            }
            break;
        }
        case IntentType::Snuff:
            if (blocking) {
                ctx.sayKey(MsgType::Heal, "combat.snuff_blocked");
            } else if (p.oil > 0) {
                ctx.sayFmt(MsgType::Damage, "combat.snuffed", {{"enemy", def.name}, {"oil", num(std::min(p.oil, def.snuffAmount))}});
                ctx.sys.light.spendOil(def.snuffAmount, ctx);
            }
            break;
        case IntentType::Drain: {
            const int amount = std::min(p.oil, blocking ? def.drainBlocked : def.drainAmount);
            if (amount <= 0) {
                ctx.sayFmt(MsgType::System, "combat.drain_empty", {{"enemy", def.name}});
                break;
            }
            ctx.sys.light.spendOil(amount, ctx);
            const int heal = amount / 2;
            e.hp = std::min(def.hp, e.hp + heal);
            e.exposed = true;
            ctx.sayFmt(MsgType::Damage, blocking ? "combat.drain_blocked" : "combat.drain",
                       {{"enemy", def.name}, {"oil", num(amount)}, {"heal", num(heal)}});
            ctx.sayFmt(MsgType::Heal, "combat.exposed", {{"enemy", def.name}});
            break;
        }
        case IntentType::Guard:
            if (blocking) ctx.sayFmt(MsgType::System, "combat.both_guard", {{"enemy", def.name}});
            break;
        case IntentType::Overheat:
            ctx.sayFmt(MsgType::System, "combat.overheat_idle", {{"enemy", def.name}});
            break;
    }
}

void CombatSystem::checkPhase(EnemyInstance& e, const EnemyDef& def, GameContext& ctx) const {
    const auto phase = static_cast<std::size_t>(e.phase);
    if (phase >= def.phases.size() || e.hp <= 0) return;
    if (static_cast<double>(e.hp) / def.hp <= def.phases[phase].hpBelow) {
        e.phase += 1;
        e.patternIndex = -1;  // advanceIntent начнёт новый паттерн с первого намерения
        if (!def.phases[phase].message.empty()) ctx.say(MsgType::Damage, def.phases[phase].message);
    }
}

void CombatSystem::advanceIntent(EnemyInstance& e, const EnemyDef& def, GameContext& ctx) const {
    const auto& pat = pattern(e, def);
    e.patternIndex = (e.patternIndex + 1) % static_cast<int>(pat.size());
    ctx.world.encounter->intent = pat[static_cast<std::size_t>(e.patternIndex)];
}

void CombatSystem::endEncounter(GameContext& ctx) const { ctx.world.encounter.reset(); }

void CombatSystem::defeat(GameContext& ctx, bool fromCombatState) const {
    RoomState& room = ctx.world.room(ctx.world.encounter->room);
    const std::size_t index = ctx.world.encounter->enemyIndex;
    const EnemyDef& def = ctx.data.enemy(room.enemies[index].def);

    ctx.say(MsgType::Heal, def.defeatText.empty() ? ctx.fmt("combat.victory", {{"enemy", def.name}}) : def.defeatText);
    if (!def.loot.empty()) {
        room.items.insert(room.items.end(), def.loot.begin(), def.loot.end());
        ctx.sayFmt(MsgType::Item, "combat.loot", {{"items", RoomDescriber::itemList(def.loot, ctx)}});
    }
    ctx.world.setFlag("defeated_" + def.id);
    ctx.world.stats.kills += 1;
    room.enemies.erase(room.enemies.begin() + static_cast<std::ptrdiff_t>(index));
    endEncounter(ctx);
    if (fromCombatState) ctx.request(StateId::Exploration);
}

}  // namespace ll
