#pragma once

#include <string>

namespace ll {

struct GameContext;

// Итоговый счёт, рейтинг и выбор эпилога (GDD, раздел 8.1).
class ScoreSystem {
public:
    int score(const GameContext& ctx) const;
    std::string rankKey(int score, const GameContext& ctx) const;  // ключ строки в strings.json
    bool trueEnding(const GameContext& ctx) const;                  // прочитаны все записки
};

}  // namespace ll
