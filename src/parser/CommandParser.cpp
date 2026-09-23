#include "parser/CommandParser.h"

#include "parser/TextNormalizer.h"

namespace ll {

ParseResult CommandParser::parse(const std::string& line, const Lexicon& lexicon) const {
    ParseResult result;
    std::vector<std::string> tokens;
    for (auto& t : TextNormalizer::tokenize(line)) {
        if (!lexicon.isNoise(t)) tokens.push_back(std::move(t));
    }
    if (tokens.empty()) {
        result.error = ParseError::Empty;
        return result;
    }

    ParsedCommand cmd;
    // Одно слово-направление («с», «вниз») — это перемещение.
    if (mode_ == ParseMode::Exploration && tokens.size() == 1) {
        if (auto dir = lexicon.direction(tokens[0])) {
            cmd.verb = Verb::Go;
            cmd.direction = dir;
            result.command = cmd;
            return result;
        }
    }

    const auto match = lexicon.matchVerb(tokens, mode_);
    if (!match) {
        result.error = ParseError::UnknownVerb;
        result.badWord = tokens[0];
        return result;
    }
    cmd.verb = match->verb;
    std::vector<std::string> rest(tokens.begin() + static_cast<std::ptrdiff_t>(match->length), tokens.end());

    if (cmd.verb == Verb::Go) {
        for (const auto& t : rest) {
            if (auto dir = lexicon.direction(t)) {
                cmd.direction = dir;
                break;
            }
        }
        if (!cmd.direction) {
            result.error = ParseError::NoDirection;
            return result;
        }
        result.command = cmd;
        return result;
    }

    // «всё» / «all» — флаг, а не часть названия.
    std::vector<std::string> words;
    for (auto& t : rest) {
        if (lexicon.isAll(t)) {
            cmd.all = true;
        } else {
            words.push_back(std::move(t));
        }
    }

    // Делим по первому предлогу: «использовать рукоять | на | насосе».
    bool afterPreposition = false;
    for (auto& w : words) {
        if (!afterPreposition && lexicon.isPreposition(w)) {
            afterPreposition = true;
            continue;
        }
        (afterPreposition ? cmd.target : cmd.object).push_back(std::move(w));
    }
    // «осмотреть на столе»: предлог в начале — значит, это всё-таки прямой объект.
    if (cmd.object.empty() && !cmd.target.empty()) std::swap(cmd.object, cmd.target);

    result.command = cmd;
    return result;
}

}  // namespace ll
