#pragma once

#include <memory>
#include <unordered_map>

#include "parser/CommandParser.h"

namespace ll {

struct GameContext;

struct ActionResult {
    bool tookTurn = false;
    bool needsClarification = false;  // объект неоднозначен — ждём уточнения от игрока
    bool clarifyTarget = false;       // уточнять нужно косвенный объект (после предлога)

    static ActionResult noTurn() { return {}; }
    static ActionResult turn() { return {true, false, false}; }
};

// Обработчик одной команды игрока (паттерн Command).
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    virtual ActionResult execute(const ParsedCommand& cmd, GameContext& ctx) = 0;
};

class CommandRegistry {
public:
    void add(Verb verb, std::unique_ptr<ICommandHandler> handler);
    ActionResult dispatch(const ParsedCommand& cmd, GameContext& ctx);
    bool has(Verb verb) const { return handlers_.count(verb) > 0; }

private:
    std::unordered_map<Verb, std::unique_ptr<ICommandHandler>> handlers_;
};

// Реестр со всеми командами режима исследования.
CommandRegistry makeExplorationCommands();

}  // namespace ll
