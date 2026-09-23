#pragma once

#include <map>
#include <memory>
#include <optional>

#include "core/IGameState.h"

namespace ll {

class GameStateMachine {
public:
    void add(StateId id, std::unique_ptr<IGameState> state);
    void change(StateId id, GameContext& ctx);  // onExit старого, onEnter нового
    IGameState& current();
    StateId currentId() const { return current_.value_or(StateId::MainMenu); }
    bool has(StateId id) const { return states_.count(id) > 0; }

private:
    std::map<StateId, std::unique_ptr<IGameState>> states_;
    std::optional<StateId> current_;
};

}  // namespace ll
