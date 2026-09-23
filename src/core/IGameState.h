#pragma once

#include <string>

#include "common/Enums.h"

namespace ll {

struct GameContext;

enum class TransitionKind { None, Change, Quit };

struct Transition {
    TransitionKind kind = TransitionKind::None;
    StateId target = StateId::MainMenu;

    static Transition none() { return {}; }
    static Transition to(StateId s) { return {TransitionKind::Change, s}; }
    static Transition quit() { return {TransitionKind::Quit, StateId::MainMenu}; }
};

// Режим игры (паттерн State): меню, исследование, бой, экраны поражения и победы.
class IGameState {
public:
    virtual ~IGameState() = default;
    virtual void onEnter(GameContext&) {}
    virtual Transition handleInput(GameContext& ctx, const std::string& line) = 0;
    virtual void onExit(GameContext&) {}
    virtual std::string prompt(const GameContext&) const { return "> "; }
};

}  // namespace ll
