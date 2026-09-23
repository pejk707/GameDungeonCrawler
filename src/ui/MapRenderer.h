#pragma once

#include <string>

namespace ll {

struct GameContext;

// ASCII-карта текущей зоны: посещённые комнаты, известные выходы, отметки.
class MapRenderer {
public:
    static std::string render(const GameContext& ctx);
};

}  // namespace ll
