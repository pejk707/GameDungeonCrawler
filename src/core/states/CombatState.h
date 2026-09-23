#pragma once

#include "core/IGameState.h"
#include "parser/CommandParser.h"
#include "parser/EntityResolver.h"

namespace ll {

class CombatState final : public IGameState {
public:
    void onEnter(GameContext& ctx) override;
    Transition handleInput(GameContext& ctx, const std::string& line) override;
    std::string prompt(const GameContext& ctx) const override;

private:
    CommandParser parser_{ParseMode::Combat};
    EntityResolver resolver_;
};

}  // namespace ll
