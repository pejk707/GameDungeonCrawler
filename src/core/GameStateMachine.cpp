#include "core/GameStateMachine.h"

#include "core/GameContext.h"

namespace ll {

void GameStateMachine::add(StateId id, std::unique_ptr<IGameState> state) { states_[id] = std::move(state); }

void GameStateMachine::change(StateId id, GameContext& ctx) {
    if (!has(id)) return;
    if (current_) {
        states_.at(*current_)->onExit(ctx);
        ctx.previousState = *current_;
    }
    current_ = id;
    ctx.currentState = id;
    states_.at(id)->onEnter(ctx);
}

IGameState& GameStateMachine::current() { return *states_.at(currentId()); }

}  // namespace ll
