#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "common/MessageLog.h"
#include "common/Random.h"
#include "core/GameContext.h"
#include "core/GameStateMachine.h"
#include "core/Systems.h"
#include "model/GameData.h"
#include "model/WorldState.h"
#include "ui/IConsole.h"
#include "ui/Renderer.h"

namespace ll {

struct Options {
    std::filesystem::path root;  // папка, в которой лежат data/ и art/
    std::optional<std::uint32_t> seed;
    bool noColor = false;
    int width = 0;  // 0 — из config.json
};

// Владеет всеми объектами игры и крутит главный цикл.
class Game {
public:
    Game(Options options, std::unique_ptr<IConsole> console);

    bool init();   // загрузить и проверить данные; при ошибке — сообщение и false
    int run();     // главный цикл: вывод → приглашение → ввод → обработка
    void start();  // перейти в главное меню
    bool step(const std::string& line);  // обработать одну строку ввода; false — выход

    GameContext& context() { return *ctx_; }
    StateId state() const { return fsm_.currentId(); }
    MessageLog& log() { return log_; }
    std::string takeOutput();  // текст всех накопленных сообщений (для тестов)

private:
    void settle(Transition t);

    Options options_;
    std::unique_ptr<IConsole> console_;
    GameData data_;
    WorldState world_;
    MessageLog log_;
    Random rng_;
    Systems systems_;
    Settings settings_;
    std::unique_ptr<GameContext> ctx_;
    GameStateMachine fsm_;
    Renderer renderer_;
    bool running_ = true;
};

}  // namespace ll
