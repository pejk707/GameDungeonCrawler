#pragma once

#include "core/IGameState.h"

namespace ll {

// Финал: Сердце разожжено — эпилог, статистика, рейтинг.
class VictoryState final : public IGameState {
public:
    void onEnter(GameContext& ctx) override;
    Transition handleInput(GameContext& ctx, const std::string& line) override;
};

}  // namespace ll
