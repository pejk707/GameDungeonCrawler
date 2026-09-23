#pragma once

#include <optional>
#include <string>
#include <vector>

#include "model/Lexicon.h"

namespace ll {

struct ParsedCommand {
    Verb verb = Verb::Look;
    std::optional<Direction> direction;  // для Go
    std::vector<std::string> object;     // «ржавый ключ» → {"ржавый", "ключ"}
    std::vector<std::string> target;     // слова после предлога: «на насосе»
    bool all = false;                    // «взять всё»
};

enum class ParseError { None, Empty, UnknownVerb, NoDirection };

struct ParseResult {
    std::optional<ParsedCommand> command;
    ParseError error = ParseError::None;
    std::string badWord;
};

// Разбор строки ввода в команду. Словарь — из commands.json, режим — исследование или бой.
class CommandParser {
public:
    explicit CommandParser(ParseMode mode) : mode_(mode) {}
    ParseResult parse(const std::string& line, const Lexicon& lexicon) const;

private:
    ParseMode mode_;
};

}  // namespace ll
