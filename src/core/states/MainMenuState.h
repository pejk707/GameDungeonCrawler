#pragma once

#include "core/IGameState.h"

namespace ll {

class MainMenuState final : public IGameState {
public:
    void onEnter(GameContext& ctx) override;
    Transition handleInput(GameContext& ctx, const std::string& line) override;

private:
    void showMenu(GameContext& ctx) const;
};

}  // namespace ll
