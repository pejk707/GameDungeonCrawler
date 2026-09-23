#include "systems/ScoreSystem.h"

#include "core/GameContext.h"

namespace ll {

int ScoreSystem::score(const GameContext& ctx) const {
    const ScoreConfig& s = ctx.data.config.score;
    const WorldState& w = ctx.world;
    return s.base + s.perOil * w.player.oil + s.perNote * static_cast<int>(w.player.readNotes.size()) -
           s.perTurn * w.stats.turns - s.perDeath * w.stats.deaths;
}

std::string ScoreSystem::rankKey(int score, const GameContext& ctx) const {
    const ScoreConfig& s = ctx.data.config.score;
    if (score >= s.master) return "rank.master";
    if (score >= s.lamplighter) return "rank.lamplighter";
    return "rank.apprentice";
}

bool ScoreSystem::trueEnding(const GameContext& ctx) const {
    return static_cast<int>(ctx.world.player.readNotes.size()) >= ctx.data.config.totalNotes;
}

}  // namespace ll
