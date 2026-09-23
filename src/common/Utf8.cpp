#include "common/Utf8.h"

namespace ll::utf8 {

std::u32string decode(std::string_view s) {
    std::u32string out;
    out.reserve(s.size());
    std::size_t i = 0;
    while (i < s.size()) {
        const auto c = static_cast<unsigned char>(s[i]);
        char32_t cp = 0;
        std::size_t len = 0;
        if (c < 0x80) {
            cp = c;
            len = 1;
        } else if ((c >> 5) == 0x6) {
            cp = c & 0x1Fu;
            len = 2;
        } else if ((c >> 4) == 0xE) {
            cp = c & 0x0Fu;
            len = 3;
        } else if ((c >> 3) == 0x1E) {
            cp = c & 0x07u;
            len = 4;
        } else {
            out.push_back(0xFFFD);
            ++i;
            continue;
        }
        if (i + len > s.size()) {
            out.push_back(0xFFFD);
            break;
        }
        bool ok = true;
        for (std::size_t k = 1; k < len; ++k) {
            const auto cc = static_cast<unsigned char>(s[i + k]);
            if ((cc >> 6) != 0x2) {
                ok = false;
                break;
            }
            cp = (cp << 6) | (cc & 0x3Fu);
        }
        if (!ok) {
            out.push_back(0xFFFD);
            ++i;
            continue;
        }
        out.push_back(cp);
        i += len;
    }
    return out;
}

std::string encode(char32_t c) {
    std::string out;
    if (c < 0x80) {
        out += static_cast<char>(c);
    } else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
    return out;
}

std::string encode(std::u32string_view s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char32_t c : s) out += encode(c);
    return out;
}

namespace {

char32_t lowerCp(char32_t c) {
    if (c >= U'A' && c <= U'Z') return c + 32;
    if (c >= 0x410 && c <= 0x42F) return c + 0x20;  // А–Я
    if (c == 0x401) return 0x451;                    // Ё
    return c;
}

char32_t upperCp(char32_t c) {
    if (c >= U'a' && c <= U'z') return c - 32;
    if (c >= 0x430 && c <= 0x44F) return c - 0x20;  // а–я
    if (c == 0x451) return 0x401;                    // ё
    return c;
}

}  // namespace

std::string toLower(std::string_view s) {
    std::u32string u = decode(s);
    for (auto& c : u) c = lowerCp(c);
    return encode(u);
}

std::string fold(std::string_view s) {
    std::u32string u = decode(s);
    for (auto& c : u) {
        c = lowerCp(c);
        if (c == 0x451) c = 0x435;  // ё → е
    }
    return encode(u);
}

std::string toUpper(std::string_view s) {
    std::u32string u = decode(s);
    for (auto& c : u) c = upperCp(c);
    return encode(u);
}

std::size_t length(std::string_view s) {
    std::size_t n = 0;
    for (char ch : s) {
        if ((static_cast<unsigned char>(ch) & 0xC0) != 0x80) ++n;
    }
    return n;
}

std::string padRight(std::string_view s, std::size_t width) {
    std::string out(s);
    const std::size_t len = length(s);
    if (len < width) out.append(width - len, ' ');
    return out;
}

std::string truncate(std::string_view s, std::size_t width) {
    std::u32string u = decode(s);
    if (u.size() <= width) return std::string(s);
    u.resize(width);
    return encode(u);
}

std::string repeat(std::string_view s, std::size_t n) {
    std::string out;
    out.reserve(s.size() * n);
    for (std::size_t i = 0; i < n; ++i) out += s;
    return out;
}

std::vector<std::string> splitLines(std::string_view text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t pos = text.find('\n', start);
        std::string line(text.substr(start, pos == std::string_view::npos ? std::string_view::npos
                                                                          : pos - start));
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
        if (pos == std::string_view::npos) break;
        start = pos + 1;
    }
    return lines;
}

std::vector<std::string> wrap(std::string_view text, std::size_t width) {
    std::vector<std::string> out;
    if (width < 8) width = 8;
    for (const std::string& paragraph : splitLines(text)) {
        const std::u32string p = decode(paragraph);
        std::size_t indent = 0;
        while (indent < p.size() && p[indent] == U' ') ++indent;
        if (p.size() <= width) {
            out.push_back(paragraph);
            continue;
        }
        const std::u32string pad(indent, U' ');
        std::u32string line = p.substr(0, indent);
        bool lineHasWord = false;
        std::size_t i = indent;
        while (i < p.size()) {
            std::size_t j = i;
            while (j < p.size() && p[j] != U' ') ++j;
            std::u32string word = p.substr(i, j - i);
            while (!word.empty()) {
                const std::size_t needed = word.size() + (lineHasWord ? 1 : 0);
                if (line.size() + needed <= width) {
                    if (lineHasWord) line += U' ';
                    line += word;
                    lineHasWord = true;
                    word.clear();
                } else if (!lineHasWord) {
                    // Слово длиннее строки — режем.
                    const std::size_t room = width > line.size() ? width - line.size() : 1;
                    line += word.substr(0, room);
                    word.erase(0, room);
                    out.push_back(encode(line));
                    line = pad;
                } else {
                    out.push_back(encode(line));
                    line = pad;
                    lineHasWord = false;
                }
            }
            i = j;
            while (i < p.size() && p[i] == U' ') ++i;
        }
        if (lineHasWord || out.empty()) out.push_back(encode(line));
    }
    return out;
}

bool startsWith(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace ll::utf8
