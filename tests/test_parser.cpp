#include "TestSupport.h"
#include "doctest/doctest.h"
#include "parser/CommandParser.h"
#include "parser/EntityResolver.h"
#include "parser/TextNormalizer.h"

using namespace ll;

namespace {

ParsedCommand parseOk(const std::string& line, ParseMode mode = ParseMode::Exploration) {
    const ParseResult r = CommandParser(mode).parse(line, test::gameData().lexicon);
    REQUIRE_MESSAGE(r.command.has_value(), line);
    return *r.command;
}

}  // namespace

TEST_CASE("нормализация: регистр, ё и пунктуация") {
    CHECK(TextNormalizer::tokenize("  Взять ВСЁ!  ") == std::vector<std::string>{"взять", "все"});
    CHECK(TextNormalizer::tokenize("«Письмо» — Орина?") == std::vector<std::string>{"письмо", "орина"});
    CHECK(TextNormalizer::tokenize("?") == std::vector<std::string>{"?"});
}

TEST_CASE("парсер: направления") {
    CHECK(parseOk("с").direction == Direction::North);
    CHECK(parseOk("вн").direction == Direction::Down);
    const auto cmd = parseOk("идти на север");
    CHECK(cmd.verb == Verb::Go);
    CHECK(cmd.direction == Direction::North);
    const auto bad = CommandParser(ParseMode::Exploration).parse("идти туда", test::gameData().lexicon);
    CHECK(bad.error == ParseError::NoDirection);
}

TEST_CASE("парсер: объекты и предлоги") {
    const auto use = parseOk("использовать рукоять на насосе");
    CHECK(use.verb == Verb::Use);
    CHECK(use.object == std::vector<std::string>{"рукоять"});
    CHECK(use.target == std::vector<std::string>{"насосе"});

    const auto all = parseOk("взять всё");
    CHECK(all.verb == Verb::Take);
    CHECK(all.all);
    CHECK(all.object.empty());

    const auto equip = parseOk("взять в руку нож");  // многословный глагол побеждает «взять»
    CHECK(equip.verb == Verb::Equip);
    CHECK(equip.object == std::vector<std::string>{"нож"});
}

TEST_CASE("парсер: фонарь и зажигание") {
    CHECK(parseOk("фонарь").verb == Verb::Lantern);
    CHECK(parseOk("потушить фонарь").verb == Verb::LanternOff);
    CHECK(parseOk("зажечь фонарь").verb == Verb::LanternOn);
    const auto light = parseOk("зажечь камин");
    CHECK(light.verb == Verb::Light);
    CHECK(light.object == std::vector<std::string>{"камин"});
    CHECK(parseOk("?").verb == Verb::Help);
}

TEST_CASE("парсер: режим боя понимает свои сокращения") {
    CHECK(parseOk("в", ParseMode::Combat).verb == Verb::Flare);
    CHECK(parseOk("а", ParseMode::Combat).verb == Verb::Attack);
    CHECK(parseOk("б", ParseMode::Combat).verb == Verb::Block);
    CHECK(parseOk("в").verb == Verb::Go);  // а при исследовании «в» — это восток
}

TEST_CASE("парсер: неизвестное слово") {
    const auto r = CommandParser(ParseMode::Exploration).parse("танцевать", test::gameData().lexicon);
    CHECK(r.error == ParseError::UnknownVerb);
    CHECK(r.badWord == "танцевать");
}

TEST_CASE("поиск объектов по основам слов") {
    CHECK(EntityResolver::score({"рукоятью"}, {"рукоят"}) == 1);
    CHECK(EntityResolver::score({"ржавый", "ключ"}, {"ключ", "ржав"}) == 2);
    CHECK(EntityResolver::score({"ключ"}, {"ключ", "ворот"}) == 1);
    CHECK(EntityResolver::score({"насос"}, {"ключ"}) == 0);
}
