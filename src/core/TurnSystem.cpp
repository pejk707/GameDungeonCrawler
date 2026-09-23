#include "core/TurnSystem.h"

#include "core/GameContext.h"
#include "core/Systems.h"

namespace ll {

void TurnSystem::endTurn(GameContext& ctx) const {
    ctx.world.stats.turns += 1;
    if (ctx.world.encounter) return;  // бой начался в этом ходу — тьма подождёт

    ctx.sys.light.tickDarkness(ctx);
    if (ctx.pendingState == StateId::GameOver) return;

    // Потревоженные спящие враги просыпаются, когда их счётчик доходит до нуля.
    RoomState& room = ctx.world.currentRoom();
    for (std::size_t i = 0; i < room.enemies.size(); ++i) {
        EnemyInstance& e = room.enemies[i];
        if (e.behavior != Behavior::Sleeping || e.wakeIn < 0) continue;
        if (e.wakeIn == 0) {
            ctx.sys.movement.wakeEnemy(i, ctx);
            break;
        }
        --e.wakeIn;
    }
    ctx.sys.hints.onTurn(ctx);
}

void TurnSystem::endCombatRound(GameContext& ctx) const { ctx.world.stats.turns += 1; }

}  // namespace ll
