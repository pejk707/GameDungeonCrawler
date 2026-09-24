#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ll {

// Консоль: всё платформенное спрятано за этим интерфейсом.
class IConsole {
public:
    virtual ~IConsole() = default;
    virtual std::optional<std::string> readLine() = 0;  // std::nullopt — конец ввода
    virtual void write(std::string_view text) = 0;       // UTF-8
    virtual bool supportsColor() const = 0;
};

// Настоящая консоль: WinConsole на Windows, PosixConsole на Linux/macOS.
std::unique_ptr<IConsole> makeSystemConsole();

// Ввод из файла сценария поверх другой консоли (режим --script): команды берутся
// из файла и выводятся на экран, затем ввод продолжается с клавиатуры.
// Пустые строки и строки на '#' — комментарии.
class ScriptConsole final : public IConsole {
public:
    ScriptConsole(std::unique_ptr<IConsole> inner, std::vector<std::string> lines, bool continueInteractive);

    std::optional<std::string> readLine() override;
    void write(std::string_view text) override { inner_->write(text); }
    bool supportsColor() const override { return inner_->supportsColor(); }

private:
    std::unique_ptr<IConsole> inner_;
    std::deque<std::string> lines_;
    bool continueInteractive_;
};

// Консоль в памяти — для тестов.
class MemoryConsole final : public IConsole {
public:
    explicit MemoryConsole(std::vector<std::string> input = {});
    std::optional<std::string> readLine() override;
    void write(std::string_view text) override { output_ += text; }
    bool supportsColor() const override { return false; }
    const std::string& output() const { return output_; }

private:
    std::deque<std::string> input_;
    std::string output_;
};

}  // namespace ll
