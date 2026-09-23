#include "systems/LightSystem.h"

#include <algorithm>
#include <string>

#include "core/GameContext.h"
#include "core/Systems.h"

namespace ll {

bool LightSystem::isRoomLit(const RoomId& room, const GameContext& ctx) const {
    return ctx.world.room(room).lit;
}

bool LightSystem::lanternBurning(const GameContext& ctx) const {
    return ctx.world.player.lanternLit && ctx.world.player.oil > 0;
}

bool LightSystem::canSee(const GameContext& ctx) const {
    return isRoomLit(ctx.world.player.location, ctx) || lanternBurning(ctx);
}

int LightSystem::moveCost(const RoomId& target, const GameContext& ctx) const {
    if (isRoomLit(target, ctx) || !lanternBurning(ctx)) return 0;
    return ctx.data.config.oilCost.moveDark;
}

void LightSystem::spendOil(int amount, GameContext& ctx) const {
    PlayerState& p = ctx.world.player;
    if (amount <= 0 || p.oil <= 0) return;
    const int before = p.oil;
    p.oil = std::max(0, p.oil - amount);
    ctx.world.stats.oilBurned += before - p.oil;
    for (int warning : ctx.data.config.oilWarnings) {
        if (before > warning && p.oil <= warning && p.oil > 0) {
            ctx.sayFmt(MsgType::Oil, "light.oil_low", {{"oil", std::to_string(p.oil)}});
            break;
        }
    }
    if (p.oil == 0 && p.lanternLit) {
        p.lanternLit = false;
        ctx.sayKey(MsgType::Damage, "light.out");
    }
}

int LightSystem::addOil(int amount, GameContext& ctx) const {
    PlayerState& p = ctx.world.player;
    const bool wasEmpty = p.oil == 0;
    const int added = std::clamp(p.maxOil - p.oil, 0, amount);
    p.oil += added;
    if (added > 0 && wasEmpty && !p.lanternLit) {
        p.lanternLit = true;
        p.darknessCounter = 0;
        ctx.sayKey(MsgType::Oil, "light.relit");
    }
    return added;
}

bool LightSystem::setLantern(bool on, GameContext& ctx) const {
    PlayerState& p = ctx.world.player;
    if (on) {
        if (p.lanternLit) {
            ctx.sayKey(MsgType::System, "light.already_on");
            return false;
        }
        if (p.oil <= 0) {
            ctx.sayKey(MsgType::System, "light.no_oil");
            return false;
        }
        p.lanternLit = true;
        p.darknessCounter = 0;
        ctx.sayKey(MsgType::Oil, "light.on");
        // Свет тревожит спящих: у игрока остаётся ровно один ход.
        RoomState& room = ctx.world.currentRoom();
        for (auto& e : room.enemies) {
            if (e.behavior == Behavior::Sleeping && e.wakeIn < 0) {
                e.wakeIn = 0;
                ctx.sayFmt(MsgType::Damage, "enemy.disturbed", {{"enemy", ctx.data.enemy(e.def).name}});
            }
        }
        return true;
    }
    if (!p.lanternLit) {
        ctx.sayKey(MsgType::System, "light.already_off");
        return false;
    }
    p.lanternLit = false;
    ctx.sayKey(MsgType::Dark, isRoomLit(p.location, ctx) ? "light.off" : "light.off_dark");
    return true;
}

void LightSystem::tickDarkness(GameContext& ctx) const {
    PlayerState& p = ctx.world.player;
    if (canSee(ctx)) {
        p.darknessCounter = 0;
        return;
    }
    p.darknessCounter += 1;
    if (p.darknessCounter >= ctx.data.config.darknessTurnsToDeath) {
        ctx.deathCause = ctx.str("death.darkness");
        ctx.request(StateId::GameOver);
        return;
    }
    const int level = std::min(p.darknessCounter, 3);
    ctx.sayKey(MsgType::Damage, "darkness." + std::to_string(level));
    ctx.sys.hints.trigger("darkness", ctx);
}

}  // namespace ll
