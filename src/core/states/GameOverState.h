#pragma once

#include "core/IGameState.h"

namespace ll {

// Экран «Фонарь погас»: загрузить сохранение, начать заново или выйти в меню.
class GameOverState final : public IGameState {
public:
    void onEnter(GameContext& ctx) override;
    Transition handleInput(GameContext& ctx, const std::string& line) override;

private:
    int deaths_ = 0;  // переживает загрузку сохранения
};

}  // namespace ll
