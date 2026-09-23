#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include "model/GameData.h"

namespace ll {

// Ошибка в файлах данных: сообщение содержит имя файла и id элемента.
class DataError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Загружает весь контент игры. root — папка, в которой лежат data/ и art/.
class DataLoader {
public:
    explicit DataLoader(std::filesystem::path root) : root_(std::move(root)) {}
    GameData loadAll() const;

    static std::string readTextFile(const std::filesystem::path& path);  // UTF-8, без BOM и \r

private:
    void loadConfig(GameData& data) const;
    void loadRooms(GameData& data) const;
    void loadItems(GameData& data) const;
    void loadEnemies(GameData& data) const;
    void loadStrings(GameData& data) const;
    void loadLexicon(GameData& data) const;
    void loadTexts(GameData& data) const;

    std::filesystem::path root_;
};

}  // namespace ll
