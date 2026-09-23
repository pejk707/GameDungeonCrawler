#include "core/GameContext.h"

#include "model/WorldFactory.h"

namespace ll {
namespace {

int priority(StateId s) {
    switch (s) {
        case StateId::GameOver: return 4;
        case StateId::Victory: return 3;
        case StateId::Combat: return 2;
        case StateId::Exploration: return 1;
        case StateId::MainMenu: return 0;
    }
    return 0;
}

}  // namespace

void GameContext::request(StateId state) {
    if (!pendingState || priority(state) > priority(*pendingState)) pendingState = state;
}

std::optional<StateId> GameContext::takePending() {
    auto p = pendingState;
    pendingState.reset();
    return p;
}

void GameContext::newGame() {
    world = WorldFactory::create(data);
    deathCause.clear();
}

}  // namespace ll
