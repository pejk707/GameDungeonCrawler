#include "systems/SaveSystem.h"

#include <filesystem>
#include <fstream>

#include "data/DataLoader.h"
#include "data/SaveCodec.h"

namespace fs = std::filesystem;

namespace ll {

bool SaveSystem::save(const WorldState& world, const std::string& path, std::string* error) const {
    try {
        const fs::path target = fs::u8path(path);
        if (target.has_parent_path()) fs::create_directories(target.parent_path());
        // Сначала пишем во временный файл, потом подменяем: так прежнее сохранение
        // не пострадает, если запись прервётся.
        const fs::path tmp = target.string() + ".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out) throw std::runtime_error("не удалось открыть файл для записи");
            out << SaveCodec::toJson(world).dump(2);
            if (!out) throw std::runtime_error("ошибка записи");
        }
        fs::rename(tmp, target);
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

std::optional<WorldState> SaveSystem::load(const GameData& data, const std::string& path, std::string* error) const {
    try {
        const std::string text = DataLoader::readTextFile(fs::u8path(path));
        return SaveCodec::fromJson(nlohmann::json::parse(text), data);
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return std::nullopt;
    }
}

bool SaveSystem::exists(const std::string& path) const {
    std::error_code ec;
    return fs::exists(fs::u8path(path), ec);
}

void SaveSystem::remove(const std::string& path) const {
    std::error_code ec;
    fs::remove(fs::u8path(path), ec);
}

}  // namespace ll
