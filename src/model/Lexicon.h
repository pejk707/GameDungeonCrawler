#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/Enums.h"

namespace ll {

enum class Verb {
    Go, Look, Examine, Take, Drop, Open, Use, Equip, Read,
    Lantern, LanternOn, LanternOff, Light, Attack, Rest,
    Inventory, Status, Map, Journal, Save, Load, Help, Hints, Quit, Eat,
    Block, Flare, Flee  // только в бою
};

enum class ParseMode { Exploration, Combat };

std::optional<Verb> verbFromKey(std::string_view key);

// Словарь парсера из commands.json. Все слова хранятся в «свёрнутой» форме (utf8::fold).
class Lexicon {
public:
    struct VerbMatch {
        Verb verb;
        std::size_t length;  // сколько слов занял глагол («взять в руку» — 3)
    };

    void addVerb(ParseMode mode, std::vector<std::string> words, Verb verb);
    void addDirection(const std::string& word, Direction dir);
    void addPreposition(const std::string& word) { prepositions_.insert(word); }
    void addNoise(const std::string& word) { noise_.insert(word); }
    void addAll(const std::string& word) { all_.insert(word); }

    // Самое длинное совпадение глагола с началом списка слов.
    std::optional<VerbMatch> matchVerb(const std::vector<std::string>& tokens, ParseMode mode) const;
    std::optional<Direction> direction(const std::string& word) const;
    bool isPreposition(const std::string& word) const { return prepositions_.count(word) > 0; }
    bool isNoise(const std::string& word) const { return noise_.count(word) > 0; }
    bool isAll(const std::string& word) const { return all_.count(word) > 0; }
    std::size_t verbCount(ParseMode mode) const;

private:
    struct Entry {
        std::vector<std::string> words;
        Verb verb;
    };
    std::vector<Entry> verbs_[2];
    std::unordered_map<std::string, Direction> directions_;
    std::unordered_set<std::string> prepositions_;
    std::unordered_set<std::string> noise_;
    std::unordered_set<std::string> all_;
};

}  // namespace ll
