#pragma once

#include <string>
#include <vector>

#include "model/GameData.h"

namespace ll {

// Проверяет перекрёстные ссылки в загруженных данных и собирает ВСЕ ошибки сразу.
class DataValidator {
public:
    std::vector<std::string> validate(const GameData& data) const;
};

}  // namespace ll
