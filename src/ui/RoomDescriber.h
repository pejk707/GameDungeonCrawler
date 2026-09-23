#pragma once

#include <string>
#include <vector>

#include "common/Ids.h"

namespace ll {

struct GameContext;

// Составляет описание комнаты: заголовок, строку состояния, текст (при свете или в темноте),
// предметы, врагов и выходы. Пишет в MessageLog, ничего не печатает сам.
class RoomDescriber {
public:
    static void describe(GameContext& ctx, bool brief = false, bool banner = false);
    static std::string statusLine(const GameContext& ctx);
    static std::string exitsLine(const GameContext& ctx);
    static std::string itemList(const std::vector<ItemId>& items, const GameContext& ctx);
    static std::string bar(int value, int max, int cells);
    static std::string header(const std::string& left, const std::string& right, int width);
};

}  // namespace ll
