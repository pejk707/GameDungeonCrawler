#include <cstdio>
#include <iostream>

#include "common/Utf8.h"
#include "ui/IConsole.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ll {
namespace {

std::optional<std::string> readStdLine() {
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

#ifdef _WIN32

std::wstring toWide(std::string_view s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string toUtf8(std::wstring_view w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0,
                                      nullptr, nullptr);
    std::string s(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr,
                        nullptr);
    return s;
}

// Windows: ввод через ReadConsoleW (иначе кириллица при вводе ломается),
// вывод через WriteConsoleW, цвета — через режим виртуального терминала.
class WinConsole final : public IConsole {
public:
    WinConsole() {
        in_ = GetStdHandle(STD_INPUT_HANDLE);
        out_ = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        DWORD mode = 0;
        outIsConsole_ = GetConsoleMode(out_, &mode) != 0;
        if (outIsConsole_) {
            color_ = SetConsoleMode(out_, mode | ENABLE_PROCESSED_OUTPUT |
                                              ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        }
        inIsConsole_ = GetConsoleMode(in_, &mode) != 0;
    }

    std::optional<std::string> readLine() override {
        if (!inIsConsole_) return readStdLine();
        std::wstring buffer;
        wchar_t chunk[256];
        for (;;) {
            DWORD read = 0;
            if (!ReadConsoleW(in_, chunk, 256, &read, nullptr) || read == 0) return std::nullopt;
            buffer.append(chunk, read);
            if (buffer.find(L'\n') != std::wstring::npos) break;
        }
        while (!buffer.empty() && (buffer.back() == L'\n' || buffer.back() == L'\r')) {
            buffer.pop_back();
        }
        if (!buffer.empty() && buffer.front() == 0x1A) return std::nullopt;  // Ctrl+Z
        return toUtf8(buffer);
    }

    void write(std::string_view text) override {
        if (outIsConsole_) {
            const std::wstring w = toWide(text);
            DWORD written = 0;
            WriteConsoleW(out_, w.data(), static_cast<DWORD>(w.size()), &written, nullptr);
        } else {
            std::fwrite(text.data(), 1, text.size(), stdout);
            std::fflush(stdout);
        }
    }

    bool supportsColor() const override { return color_; }

private:
    HANDLE in_ = nullptr;
    HANDLE out_ = nullptr;
    bool inIsConsole_ = false;
    bool outIsConsole_ = false;
    bool color_ = false;
};

#else

class PosixConsole final : public IConsole {
public:
    PosixConsole() : color_(isatty(STDOUT_FILENO) != 0) {}
    std::optional<std::string> readLine() override { return readStdLine(); }
    void write(std::string_view text) override { std::cout << text << std::flush; }
    bool supportsColor() const override { return color_; }

private:
    bool color_;
};

#endif

}  // namespace

std::unique_ptr<IConsole> makeSystemConsole() {
#ifdef _WIN32
    return std::make_unique<WinConsole>();
#else
    return std::make_unique<PosixConsole>();
#endif
}

ScriptConsole::ScriptConsole(std::unique_ptr<IConsole> inner, std::vector<std::string> lines,
                             bool continueInteractive)
    : inner_(std::move(inner)), lines_(lines.begin(), lines.end()), continueInteractive_(continueInteractive) {}

std::optional<std::string> ScriptConsole::readLine() {
    while (!lines_.empty()) {
        std::string line = lines_.front();
        lines_.pop_front();
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || utf8::startsWith(line, "#")) continue;
        inner_->write(line + "\n");
        return line;
    }
    if (continueInteractive_) return inner_->readLine();
    return std::nullopt;
}

MemoryConsole::MemoryConsole(std::vector<std::string> input) : input_(input.begin(), input.end()) {}

std::optional<std::string> MemoryConsole::readLine() {
    if (input_.empty()) return std::nullopt;
    std::string line = input_.front();
    input_.pop_front();
    output_ += line + "\n";
    return line;
}

}  // namespace ll
