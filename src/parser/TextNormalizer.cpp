#include "parser/TextNormalizer.h"

#include <sstream>

#include "common/Utf8.h"

namespace ll {
namespace {

bool isWordChar(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'0' && c <= U'9') || (c >= 0x430 && c <= 0x44F) ||
           c == U'-';
}

}  // namespace

std::string TextNormalizer::normalize(std::string_view line) {
    std::u32string u = utf8::decode(utf8::fold(line));
    for (auto& c : u) {
        if (!isWordChar(c)) c = U' ';
    }
    return utf8::encode(u);
}

std::vector<std::string> TextNormalizer::tokenize(std::string_view line) {
    std::vector<std::string> tokens;
    std::istringstream in(normalize(line));
    std::string word;
    while (in >> word) {
        // Дефисы по краям — это пунктуация («— так»), внутри слова — часть слова.
        while (!word.empty() && word.front() == '-') word.erase(word.begin());
        while (!word.empty() && word.back() == '-') word.pop_back();
        if (!word.empty()) tokens.push_back(word);
    }
    if (tokens.empty() && line.find('?') != std::string_view::npos) tokens.push_back("?");
    return tokens;
}

}  // namespace ll
