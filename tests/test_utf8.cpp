#include "common/MessageLog.h"
#include "common/Utf8.h"
#include "doctest/doctest.h"
#include "ui/Renderer.h"

using namespace ll;

TEST_CASE("utf8: длина и регистр кириллицы") {
    CHECK(utf8::length("Ёжик") == 4);
    CHECK(utf8::toLower("ПРИВЕТ, Ёжик!") == "привет, ёжик!");
    CHECK(utf8::toUpper("сильный удар") == "СИЛЬНЫЙ УДАР");
    CHECK(utf8::encode(utf8::decode("Фонарь")) == "Фонарь");
}

TEST_CASE("utf8: перенос строк считает символы, а не байты") {
    const auto lines = utf8::wrap("Свет фонаря выхватывает из темноты огромный насос", 20);
    REQUIRE(lines.size() == 4);
    for (const auto& line : lines) CHECK(utf8::length(line) <= 20);
    CHECK(lines[0] == "Свет фонаря");
}

TEST_CASE("utf8: отступ сохраняется при переносе") {
    const auto lines = utf8::wrap("  » Намерение врага очень длинное и не влезает", 20);
    REQUIRE(lines.size() >= 2);
    CHECK(utf8::startsWith(lines[1], "  "));
}

TEST_CASE("renderer: без цвета выводит чистый текст") {
    Renderer r(false, 40);
    CHECK(r.render({MsgType::Damage, "Урон: 4"}) == "Урон: 4\n");
    CHECK(r.render({MsgType::Text, ""}) == "\n");
    Renderer colored(true, 40);
    CHECK(colored.render({MsgType::Damage, "x"}).find("\x1b[91m") != std::string::npos);
}
