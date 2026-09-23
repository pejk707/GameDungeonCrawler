#include "model/GameData.h"

namespace ll {

const ObjectDef* RoomDef::findObject(const ObjectId& objectId) const {
    for (const auto& o : objects) {
        if (o.id == objectId) return &o;
    }
    return nullptr;
}

const ObjectDef* RoomDef::brazier() const {
    for (const auto& o : objects) {
        if (o.kind == ObjectKind::Brazier) return &o;
    }
    return nullptr;
}

std::string StringTable::get(const std::string& key) const {
    auto it = map_.find(key);
    if (it == map_.end()) return "[" + key + "]";
    return it->second;
}

std::string StringTable::format(const std::string& key, const Args& args) const {
    std::string s = get(key);
    for (const auto& [name, value] : args) {
        const std::string token = "{" + name + "}";
        std::size_t pos = 0;
        while ((pos = s.find(token, pos)) != std::string::npos) {
            s.replace(pos, token.size(), value);
            pos += value.size();
        }
    }
    return s;
}

std::string TextLibrary::get(const std::string& path) const {
    auto it = texts_.find(path);
    return it == texts_.end() ? std::string{} : it->second;
}

const RoomDef* GameData::findRoom(const RoomId& id) const {
    auto it = rooms.find(id);
    return it == rooms.end() ? nullptr : &it->second;
}

const ItemDef* GameData::findItem(const ItemId& id) const {
    auto it = items.find(id);
    return it == items.end() ? nullptr : &it->second;
}

const EnemyDef* GameData::findEnemy(const EnemyId& id) const {
    auto it = enemies.find(id);
    return it == enemies.end() ? nullptr : &it->second;
}

const ZoneDef* GameData::findZone(const ZoneId& id) const {
    auto it = zones.find(id);
    return it == zones.end() ? nullptr : &it->second;
}

}  // namespace ll
