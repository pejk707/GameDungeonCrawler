#pragma once

#include <optional>
#include <string>

#include "model/GameData.h"
#include "model/WorldState.h"

namespace ll {

// Сохранение и загрузка мира в saves/save.json.
class SaveSystem {
public:
    bool save(const WorldState& world, const std::string& path, std::string* error = nullptr) const;
    std::optional<WorldState> load(const GameData& data, const std::string& path, std::string* error = nullptr) const;
    bool exists(const std::string& path) const;
    void remove(const std::string& path) const;
};

}  // namespace ll
