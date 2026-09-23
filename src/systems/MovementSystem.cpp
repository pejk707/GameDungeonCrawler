#include "systems/MovementSystem.h"

#include <string>

#include "core/GameContext.h"
#include "core/Systems.h"

namespace ll {

const ExitDef* MovementSystem::findExit(const RoomId& room, Direction dir, const GameContext& ctx) const {
    const RoomDef& def = ctx.data.room(room);
    auto it = def.exits.find(dir);
    if (it == def.exits.end()) return nullptr;
    if (it->second.hidden && !ctx.world.room(room).revealedExits.count(dir)) return nullptr;
    return &it->second;
}

bool MovementSystem::isLocked(const RoomId& room, const ExitDef& exit, const GameContext& ctx) const {
    return exit.lock.has_value() && !ctx.world.room(room).unlockedExits.count(exit.dir);
}

void MovementSystem::enter(const RoomId& target, GameContext& ctx) const {
    PlayerState& p = ctx.world.player;
    // Потревоженные враги успокаиваются, когда игрок уходит.
    for (auto& e : ctx.world.currentRoom().enemies) {
        if (e.behavior == Behavior::Sleeping) e.wakeIn = -1;
    }
    ctx.sys.light.spendOil(ctx.sys.light.moveCost(target, ctx), ctx);
    p.previousLocation = p.location;
    p.location = target;
}

MoveResult MovementSystem::move(Direction dir, GameContext& ctx) const {
    MoveResult result;
    const RoomId from = ctx.world.player.location;
    const ExitDef* exit = findExit(from, dir, ctx);
    if (!exit) {
        if (ctx.sys.light.canSee(ctx)) {
            ctx.sayFmt(MsgType::System, "move.no_way", {{"dir", ctx.str("dir." + std::string(toKey(dir)))}});
        } else {
            ctx.sayKey(MsgType::Dark, "move.bump");
            result.tookTurn = true;
        }
        return result;
    }
    if (isLocked(from, *exit, ctx)) {
        if (!ctx.sys.interaction.allMet(exit->lock->conditions, ctx)) {
            ctx.say(MsgType::System, exit->lock->lockedMessage.empty() ? ctx.str("move.locked")
                                                                        : exit->lock->lockedMessage);
            return result;
        }
        ctx.world.room(from).unlockedExits.insert(dir);
        if (!exit->lock->unlockMessage.empty()) ctx.say(MsgType::Item, exit->lock->unlockMessage);
    }

    const RoomDef& target = ctx.data.room(exit->to);
    const RoomDef& source = ctx.data.room(from);
    result.newZone = target.zone != source.zone;
    if (result.newZone) {
        for (const auto& [id, rs] : ctx.world.rooms) {
            if (rs.visited && ctx.data.room(id).zone == target.zone) {
                result.newZone = false;
                break;
            }
        }
    }
    enter(exit->to, ctx);
    ctx.world.currentRoom().visited = true;
    result.moved = true;
    result.tookTurn = true;
    return result;
}

bool MovementSystem::engageAggressive(GameContext& ctx) const {
    RoomState& room = ctx.world.currentRoom();
    for (std::size_t i = 0; i < room.enemies.size(); ++i) {
        if (room.enemies[i].behavior == Behavior::Aggressive) {
            ctx.sys.combat.begin(i, false, ctx);
            return true;
        }
    }
    return false;
}

void MovementSystem::arrive(GameContext& ctx) const {
    if (engageAggressive(ctx)) return;  // агрессивный враг нападает сразу
    const bool see = ctx.sys.light.canSee(ctx);
    RoomState& room = ctx.world.currentRoom();
    for (std::size_t i = 0; i < room.enemies.size(); ++i) {
        EnemyInstance& e = room.enemies[i];
        if (e.behavior == Behavior::Sleeping) {
            if (see && e.wakeIn < 0) {
                e.wakeIn = 1;  // сам переход — это ход, поэтому у игрока остаётся ровно один ход
                ctx.sayFmt(MsgType::Damage, "enemy.sleeping_disturbed", {{"enemy", ctx.data.enemy(e.def).name}});
                ctx.sys.hints.trigger("sleeping", ctx);
            }
        }
    }
    const RoomDef& def = ctx.data.room(ctx.world.player.location);
    if (!see) {
        ctx.sys.hints.trigger("no_light", ctx);
    } else if (!room.lit) {
        ctx.sys.hints.trigger("dark_room", ctx);
    }
    if (def.brazier() && !room.lit && see) ctx.sys.hints.trigger("brazier", ctx);
}

bool MovementSystem::retreat(GameContext& ctx) const {
    PlayerState& p = ctx.world.player;
    if (p.previousLocation.empty() || p.previousLocation == p.location) return false;
    enter(p.previousLocation, ctx);
    return true;
}

void MovementSystem::wakeEnemy(std::size_t index, GameContext& ctx) const {
    RoomState& room = ctx.world.currentRoom();
    if (index >= room.enemies.size()) return;
    EnemyInstance& e = room.enemies[index];
    e.behavior = Behavior::Aggressive;
    e.wakeIn = -1;
    ctx.sayFmt(MsgType::Damage, "enemy.woke", {{"enemy", ctx.data.enemy(e.def).name}});
    ctx.sys.combat.begin(index, false, ctx);
}

}  // namespace ll
