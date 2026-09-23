#include "model/Lexicon.h"

#include <array>
#include <utility>

namespace ll {
namespace {

constexpr std::array<std::pair<Verb, std::string_view>, 28> kVerbKeys{{
    {Verb::Go, "go"},           {Verb::Look, "look"},         {Verb::Examine, "examine"},
    {Verb::Take, "take"},       {Verb::Drop, "drop"},         {Verb::Open, "open"},
    {Verb::Use, "use"},         {Verb::Equip, "equip"},       {Verb::Read, "read"},
    {Verb::Lantern, "lantern"}, {Verb::LanternOn, "lantern_on"}, {Verb::LanternOff, "lantern_off"},
    {Verb::Light, "light"},     {Verb::Attack, "attack"},     {Verb::Rest, "rest"},
    {Verb::Inventory, "inventory"}, {Verb::Status, "status"}, {Verb::Map, "map"},
    {Verb::Journal, "journal"}, {Verb::Save, "save"},         {Verb::Load, "load"},
    {Verb::Help, "help"},       {Verb::Hints, "hints"},       {Verb::Quit, "quit"},
    {Verb::Eat, "eat"},         {Verb::Block, "block"},       {Verb::Flare, "flare"},
    {Verb::Flee, "flee"},
}};

std::size_t modeIndex(ParseMode mode) { return mode == ParseMode::Exploration ? 0 : 1; }

}  // namespace

std::optional<Verb> verbFromKey(std::string_view key) {
    for (const auto& [verb, name] : kVerbKeys) {
        if (name == key) return verb;
    }
    return std::nullopt;
}

void Lexicon::addVerb(ParseMode mode, std::vector<std::string> words, Verb verb) {
    if (words.empty()) return;
    verbs_[modeIndex(mode)].push_back({std::move(words), verb});
}

void Lexicon::addDirection(const std::string& word, Direction dir) { directions_[word] = dir; }

std::optional<Lexicon::VerbMatch> Lexicon::matchVerb(const std::vector<std::string>& tokens,
                                                     ParseMode mode) const {
    std::optional<VerbMatch> best;
    for (const Entry& e : verbs_[modeIndex(mode)]) {
        if (e.words.size() > tokens.size()) continue;
        bool match = true;
        for (std::size_t i = 0; i < e.words.size(); ++i) {
            if (e.words[i] != tokens[i]) {
                match = false;
                break;
            }
        }
        if (match && (!best || e.words.size() > best->length)) best = VerbMatch{e.verb, e.words.size()};
    }
    return best;
}

std::optional<Direction> Lexicon::direction(const std::string& word) const {
    auto it = directions_.find(word);
    if (it == directions_.end()) return std::nullopt;
    return it->second;
}

std::size_t Lexicon::verbCount(ParseMode mode) const { return verbs_[modeIndex(mode)].size(); }

}  // namespace ll
