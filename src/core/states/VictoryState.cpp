#include "core/states/VictoryState.h"

#include <string>

#include "core/GameContext.h"
#include "core/Systems.h"

namespace ll {

void VictoryState::onEnter(GameContext& ctx) {
    const GameConfig& cfg = ctx.data.config;
    const WorldState& w = ctx.world;
    const bool trueEnding = ctx.sys.score.trueEnding(ctx);
    const int score = ctx.sys.score.score(ctx);

    ctx.log.blank();
    ctx.say(MsgType::Art, ctx.data.texts.get(cfg.texts.victory));
    ctx.log.blank();
    ctx.say(MsgType::Text, ctx.data.texts.get(trueEnding ? cfg.texts.epilogueB : cfg.texts.epilogueA));
    ctx.log.blank();
    ctx.sayKey(MsgType::Title, "victory.stats_title");
    ctx.sayFmt(MsgType::Status, "victory.stats",
               {{"turns", std::to_string(w.stats.turns)},
                {"oil", std::to_string(w.stats.oilBurned)},
                {"notes", std::to_string(w.player.readNotes.size())},
                {"total", std::to_string(cfg.totalNotes)},
                {"deaths", std::to_string(w.stats.deaths)},
                {"kills", std::to_string(w.stats.kills)}});
    ctx.sayFmt(MsgType::Title, "victory.rank",
               {{"rank", ctx.str(ctx.sys.score.rankKey(score, ctx))}, {"score", std::to_string(score)}});
    if (!trueEnding) ctx.sayKey(MsgType::Hint, "victory.more_notes");
    ctx.log.blank();
    ctx.sayKey(MsgType::System, "victory.continue");

    // Пройденную игру не продолжить из меню.
    ctx.sys.save.remove(ctx.settings.savePath);
}

Transition VictoryState::handleInput(GameContext&, const std::string&) { return Transition::to(StateId::MainMenu); }

}  // namespace ll
