#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// Минимальная работа с UTF-8: игра хранит весь текст в UTF-8,
// а длину строк и перенос считает в кодовых точках, а не в байтах.
namespace ll::utf8 {

std::u32string decode(std::string_view s);  // неверные байты → U+FFFD
std::string encode(std::u32string_view s);
std::string encode(char32_t c);

std::string toLower(std::string_view s);  // латиница и кириллица, включая Ё
std::string toUpper(std::string_view s);
std::string fold(std::string_view s);     // нижний регистр + «ё» → «е»: форма для сравнения слов
std::string capitalize(std::string_view s);  // первая буква — заглавная
std::size_t length(std::string_view s);

std::string padRight(std::string_view s, std::size_t width);
std::string truncate(std::string_view s, std::size_t width);
std::string repeat(std::string_view s, std::size_t n);

// Перенос по словам. Абзацы разделяются '\n'; отступ первой строки абзаца
// сохраняется для его продолжения.
std::vector<std::string> wrap(std::string_view text, std::size_t width);

std::vector<std::string> splitLines(std::string_view text);
bool startsWith(std::string_view s, std::string_view prefix);

}  // namespace ll::utf8
