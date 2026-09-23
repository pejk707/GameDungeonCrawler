#pragma once

#include "data/DataLoader.h"
#include "model/GameData.h"

namespace ll::test {

// Настоящие данные игры из папки исходников (загружаются один раз на все тесты).
inline const GameData& gameData() {
    static const GameData data = DataLoader(LAMPLIGHTER_SOURCE_DIR).loadAll();
    return data;
}

}  // namespace ll::test
