#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "model/Defs.h"
#include "model/Lexicon.h"

namespace ll {

struct ZoneDef {
    ZoneId id;
    std::string name;
    std::string banner;  // путь к ASCII-баннеру
};

// Строки интерфейса из strings.json. Подстановки вида {item}, {dmg}.
class StringTable {
public:
    using Args = std::vector<std::pair<std::string, std::string>>;

    void set(std::string key, std::string value) { map_[std::move(key)] = std::move(value); }
    bool has(const std::string& key) const { return map_.count(key) > 0; }
    std::string get(const std::string& key) const;  // нет ключа → "[key]"
    std::string format(const std::string& key, const Args& args) const;
    std::size_t size() const { return map_.size(); }

private:
    std::unordered_map<std::string, std::string> map_;
};

// Тексты из отдельных файлов: ASCII-арт, записки, эпилоги. Ключ — путь от корня игры.
class TextLibrary {
public:
    void set(std::string path, std::string text) { texts_[std::move(path)] = std::move(text); }
    bool has(const std::string& path) const { return texts_.count(path) > 0; }
    std::string get(const std::string& path) const;
    std::size_t size() const { return texts_.size(); }

private:
    std::unordered_map<std::string, std::string> texts_;
};

// Корень всех определений. После загрузки используется только через const&.
struct GameData {
    GameConfig config;
    std::map<ZoneId, ZoneDef> zones;
    std::unordered_map<RoomId, RoomDef> rooms;
    std::vector<RoomId> roomOrder;  // порядок из файла
    std::unordered_map<ItemId, ItemDef> items;
    std::unordered_map<EnemyId, EnemyDef> enemies;
    StringTable strings;
    Lexicon lexicon;
    TextLibrary texts;

    const RoomDef& room(const RoomId& id) const { return rooms.at(id); }
    const ItemDef& item(const ItemId& id) const { return items.at(id); }
    const EnemyDef& enemy(const EnemyId& id) const { return enemies.at(id); }
    const RoomDef* findRoom(const RoomId& id) const;
    const ItemDef* findItem(const ItemId& id) const;
    const EnemyDef* findEnemy(const EnemyId& id) const;
    const ZoneDef* findZone(const ZoneId& id) const;
};

}  // namespace ll
