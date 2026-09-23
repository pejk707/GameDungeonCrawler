#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ll {

// Приводит ввод игрока к единой форме: нижний регистр, «ё» → «е», без пунктуации.
class TextNormalizer {
public:
    static std::string normalize(std::string_view line);
    static std::vector<std::string> tokenize(std::string_view line);
};

}  // namespace ll
