#pragma once

#include <memory>
#include <string>

#include "core/Game.h"
#include "doctest/doctest.h"
#include "ui/IConsole.h"

namespace ll::test {

// Игра целиком, но с консолью в памяти и фиксированным seed.
class GameHarness {
public:
    explicit GameHarness(std::uint32_t seed = 1)
        : game_(Options{LAMPLIGHTER_SOURCE_DIR, seed, true, 0}, std::make_unique<MemoryConsole>()) {
        REQUIRE(game_.init());
        game_.start();
        game_.takeOutput();
    }

    // Выполнить команду и вернуть всё, что игра ответила.
    std::string cmd(const std::string& line) {
        game_.step(line);
        return game_.takeOutput();
    }

    // Перенести игрока в комнату (для тестов отдельных механик).
    void teleport(const RoomId& room) {
        player().previousLocation = player().location;
        player().location = room;
        world().room(room).visited = true;
    }

    Game& game() { return game_; }
    GameContext& ctx() { return game_.context(); }
    WorldState& world() { return game_.context().world; }
    PlayerState& player() { return game_.context().world.player; }

private:
    Game game_;
};

inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

}  // namespace ll::test
