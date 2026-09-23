#pragma once

#include "model/GameData.h"
#include "model/WorldState.h"
#include "nlohmann/json.hpp"

namespace ll {

// WorldState ↔ JSON. Бой (encounter) не сохраняется: сохраняться можно только у жаровни.
class SaveCodec {
public:
    static constexpr int kVersion = 1;
    static nlohmann::json toJson(const WorldState& world);
    // Бросает DataError, если файл повреждён или ссылается на несуществующие id.
    static WorldState fromJson(const nlohmann::json& j, const GameData& data);
};

}  // namespace ll
