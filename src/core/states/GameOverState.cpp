#include "core/states/GameOverState.h"

#include "core/GameContext.h"
#include "core/Systems.h"
#include "parser/TextNormalizer.h"

namespace ll {

void GameOverState::onEnter(GameContext& ctx) {
    ctx.world.encounter.reset();
    deaths_ = ctx.world.stats.deaths + 1;
    ctx.world.stats.deaths = deaths_;
    ctx.log.blank();
    ctx.say(MsgType::Damage, ctx.deathCause.empty() ? ctx.str("death.darkness") : ctx.deathCause);
    ctx.say(MsgType::Art, ctx.data.texts.get(ctx.data.config.texts.death));
    ctx.log.blank();
    ctx.sayKey(MsgType::Title, "gameover.options");
}

Transition GameOverState::handleInput(GameContext& ctx, const std::string& line) {
    const auto tokens = TextNormalizer::tokenize(line);
    const std::string w = tokens.empty() ? std::string{} : tokens[0];
    if (w == "1" || w == "загрузить" || w == "load") {
        if (!ctx.loadGame()) return Transition::none();
        ctx.world.stats.deaths = deaths_;  // смерть учитывается и после загрузки
        return Transition::to(StateId::Exploration);
    }
    if (w == "2" || w == "новая" || w == "new") {
        ctx.newGame();
        ctx.world.stats.deaths = deaths_;
        ctx.say(MsgType::Text, ctx.data.texts.get(ctx.data.config.texts.intro));
        return Transition::to(StateId::Exploration);
    }
    if (w == "3" || w == "меню" || w == "выход" || w == "menu") return Transition::to(StateId::MainMenu);
    ctx.sayKey(MsgType::Title, "gameover.options");
    return Transition::none();
}

}  // namespace ll
