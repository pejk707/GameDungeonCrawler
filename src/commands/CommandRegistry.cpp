#include "commands/ICommandHandler.h"
#include "core/GameContext.h"

namespace ll {

void CommandRegistry::add(Verb verb, std::unique_ptr<ICommandHandler> handler) {
    handlers_[verb] = std::move(handler);
}

ActionResult CommandRegistry::dispatch(const ParsedCommand& cmd, GameContext& ctx) {
    auto it = handlers_.find(cmd.verb);
    if (it == handlers_.end()) {
        ctx.sayKey(MsgType::System, "parse.not_now");
        return ActionResult::noTurn();
    }
    return it->second->execute(cmd, ctx);
}

}  // namespace ll
