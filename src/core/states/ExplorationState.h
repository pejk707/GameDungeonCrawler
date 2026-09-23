#pragma once

#include <optional>

#include "commands/ICommandHandler.h"
#include "core/IGameState.h"
#include "parser/CommandParser.h"

namespace ll {

class ExplorationState final : public IGameState {
public:
    ExplorationState() : registry_(makeExplorationCommands()) {}
    void onEnter(GameContext& ctx) override;
    Transition handleInput(GameContext& ctx, const std::string& line) override;

private:
    Transition execute(const ParsedCommand& cmd, GameContext& ctx);

    CommandParser parser_{ParseMode::Exploration};
    CommandRegistry registry_;
    std::optional<ParsedCommand> pending_;  // команда, ждущая уточнения («Какой ключ?»)
    bool pendingTarget_ = false;
};

}  // namespace ll
