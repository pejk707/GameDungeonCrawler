#include "core/states/ExplorationState.h"

#include "core/GameContext.h"
#include "core/Systems.h"
#include "parser/TextNormalizer.h"
#include "ui/RoomDescriber.h"

namespace ll {

void ExplorationState::onEnter(GameContext& ctx) {
    pending_.reset();
    const bool afterCombat = ctx.previousState == StateId::Combat;
    if (afterCombat) {
        // После победы — коротко (лут и выходы); после бегства — новая комната целиком.
        const bool sameRoom = ctx.world.player.location == ctx.lastCombatRoom;
        RoomDescriber::describe(ctx, sameRoom, false);
        ctx.sys.movement.engageAggressive(ctx);  // в комнате мог остаться ещё один враг
        return;
    }
    RoomDescriber::describe(ctx, false, ctx.world.stats.turns == 0);
    ctx.sys.movement.arrive(ctx);
}

Transition ExplorationState::handleInput(GameContext& ctx, const std::string& line) {
    ParseResult parsed = parser_.parse(line, ctx.data.lexicon);

    // Ответ на уточняющий вопрос: если это не новая команда — дописываем слова к отложенной.
    if (pending_) {
        ParsedCommand cmd = *pending_;
        pending_.reset();
        if (!parsed.command && parsed.error == ParseError::UnknownVerb) {
            auto& slot = pendingTarget_ ? cmd.target : cmd.object;
            for (auto& w : TextNormalizer::tokenize(line)) slot.push_back(std::move(w));
            return execute(cmd, ctx);
        }
    }

    if (!parsed.command) {
        switch (parsed.error) {
            case ParseError::UnknownVerb:
                ctx.sayFmt(MsgType::System, "parse.unknown_verb", {{"word", parsed.badWord}});
                break;
            case ParseError::NoDirection:
                ctx.sayKey(MsgType::System, "parse.no_direction");
                break;
            default:
                ctx.sayKey(MsgType::System, "parse.empty");
        }
        return Transition::none();
    }
    return execute(*parsed.command, ctx);
}

Transition ExplorationState::execute(const ParsedCommand& cmd, GameContext& ctx) {
    const ActionResult result = registry_.dispatch(cmd, ctx);
    if (result.needsClarification) {
        pending_ = cmd;
        pendingTarget_ = result.clarifyTarget;
        return Transition::none();
    }
    if (result.tookTurn) ctx.sys.turn.endTurn(ctx);
    if (auto next = ctx.takePending()) return Transition::to(*next);
    return Transition::none();
}

}  // namespace ll
