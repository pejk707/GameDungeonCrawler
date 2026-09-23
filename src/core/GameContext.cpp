#include "core/GameContext.h"

#include "common/Utf8.h"
#include "core/Systems.h"
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

void GameContext::say(MsgType type, std::string text) {
    const bool preformatted = type == MsgType::Art || type == MsgType::Map || type == MsgType::Status;
    log.push(type, preformatted ? std::move(text) : utf8::capitalize(text));
}

void GameContext::newGame() {
    world = WorldFactory::create(data);
    deathCause.clear();
}

bool GameContext::saveGame() {
    std::string error;
    if (sys.save.save(world, settings.savePath, &error)) return true;
    sayFmt(MsgType::Damage, "save.failed", {{"error", error}});
    return false;
}

bool GameContext::loadGame() {
    if (!sys.save.exists(settings.savePath)) {
        sayKey(MsgType::System, "load.no_save");
        return false;
    }
    std::string error;
    auto loaded = sys.save.load(data, settings.savePath, &error);
    if (!loaded) {
        sayFmt(MsgType::Damage, "load.corrupted", {{"error", error}});
        return false;
    }
    world = std::move(*loaded);  // тот же объект: ссылки на world остаются валидными
    deathCause.clear();
    sayKey(MsgType::Oil, "load.ok");
    return true;
}

}  // namespace ll
