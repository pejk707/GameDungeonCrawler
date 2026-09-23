#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "core/Game.h"
#include "ui/IConsole.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

fs::path executableDir(const char* argv0) {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (n > 0 && n < MAX_PATH) return fs::path(buffer).parent_path();
#endif
    std::error_code ec;
    const fs::path p = fs::absolute(argv0, ec);
    return ec ? fs::current_path() : p.parent_path();
}

// Ищем папку с data/: рядом с программой, в текущей папке, на уровень выше.
fs::path findRoot(const fs::path& exeDir) {
    for (const fs::path& candidate : {exeDir, fs::current_path(), exeDir.parent_path()}) {
        if (fs::exists(candidate / "data" / "config.json")) return candidate;
    }
    return fs::current_path();
}

std::vector<std::string> readLines(const fs::path& file) {
    std::vector<std::string> lines;
    std::ifstream in(file, std::ios::binary);
    std::string line;
    while (std::getline(in, line)) {
        if (lines.empty() && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

void printUsage() {
    std::cout << "Usage: lamplighter [--data DIR] [--seed N] [--no-color] [--width N]\n"
                 "                   [--script FILE] [--demo FILE] [--demo-speed MS]\n"
                 "  --script FILE   read commands from FILE, then continue interactively\n"
                 "  --demo FILE     like --script, but types commands slowly (for recording video)\n";
}

}  // namespace

int main(int argc, char** argv) {
    ll::Options options;
    std::string scriptFile;
    bool demo = false;
    int demoCharMs = 45;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : std::string{}; };
        if (arg == "--data") {
            options.root = next();
        } else if (arg == "--seed") {
            options.seed = static_cast<std::uint32_t>(std::stoul(next()));
        } else if (arg == "--no-color") {
            options.noColor = true;
        } else if (arg == "--width") {
            options.width = std::stoi(next());
        } else if (arg == "--script") {
            scriptFile = next();
        } else if (arg == "--demo") {
            scriptFile = next();
            demo = true;
        } else if (arg == "--demo-speed") {
            demoCharMs = std::stoi(next());
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage();
            return 2;
        }
    }
    if (options.root.empty()) options.root = findRoot(executableDir(argv[0]));
    if (demo && !options.seed) options.seed = 7;  // демо всегда проигрывается одинаково

    std::unique_ptr<ll::IConsole> console = ll::makeSystemConsole();
    if (!scriptFile.empty()) {
        auto lines = readLines(scriptFile);
        if (lines.empty()) {
            std::cerr << "Script file is empty or missing: " << scriptFile << "\n";
            return 2;
        }
        ll::ScriptConsole::Timing timing;
        if (demo) timing = {demoCharMs, 700, 300};
        console = std::make_unique<ll::ScriptConsole>(std::move(console), std::move(lines), timing, true);
    }

    ll::Game game(options, std::move(console));
    game.init();
    return game.run();
}
