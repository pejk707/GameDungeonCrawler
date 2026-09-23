#pragma once

#include <optional>
#include <string>

#include "common/Enums.h"
#include "common/MessageLog.h"
#include "common/Random.h"
#include "model/GameData.h"
#include "model/WorldState.h"

namespace ll {

struct Systems;

struct Settings {
    bool hints = true;
    std::string savePath;
};

// «Рюкзак» ссылок, который получает каждый обработчик и каждая система.
struct GameContext {
    GameContext(const GameData& d, WorldState& w, MessageLog& l, IRandom& r, Systems& s, Settings& st)
        : data(d), world(w), log(l), rng(r), sys(s), settings(st) {}

    const GameData& data;
    WorldState& world;
    MessageLog& log;
    IRandom& rng;
    Systems& sys;
    Settings& settings;

    StateId currentState = StateId::MainMenu;
    StateId previousState = StateId::MainMenu;
    std::optional<StateId> pendingState;  // запрос смены режима от систем
    std::string deathCause;
    RoomId lastCombatRoom;  // где начался последний бой (после бегства комнату описываем заново)

    // Запрос перехода. Если запросов несколько, побеждает самый важный: GameOver > Victory > Combat.
    void request(StateId state);
    std::optional<StateId> takePending();

    void newGame();
    bool saveGame();  // с сообщением об ошибке, если записать не удалось
    bool loadGame();  // заменяет world сохранённым состоянием; false — сохранения нет или оно повреждено

    // Короткие помощники для сообщений.
    std::string str(const std::string& key) const { return data.strings.get(key); }
    std::string fmt(const std::string& key, const StringTable::Args& args) const {
        return data.strings.format(key, args);
    }
    // Сообщения начинаются с заглавной буквы: «{enemy} бьёт вас» → «Пещерная крыса бьёт вас».
    void say(MsgType type, std::string text);
    void sayKey(MsgType type, const std::string& key) { say(type, str(key)); }
    void sayFmt(MsgType type, const std::string& key, const StringTable::Args& args) { say(type, fmt(key, args)); }
};

}  // namespace ll
