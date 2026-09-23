#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/Enums.h"
#include "common/Ids.h"

namespace ll {

// Состояние мира: всё, что меняется во время игры. Сохранение — это сериализация WorldState.

class Inventory {
public:
    void add(const ItemId& id, int count = 1);
    bool remove(const ItemId& id, int count = 1);
    bool has(const ItemId& id) const { return count(id) > 0; }
    int count(const ItemId& id) const;
    const std::vector<std::pair<ItemId, int>>& entries() const { return entries_; }
    bool empty() const { return entries_.empty(); }

private:
    std::vector<std::pair<ItemId, int>> entries_;  // порядок — как предметы были найдены
};

struct Equipment {
    std::optional<ItemId> weapon;
    std::optional<ItemId> body;
    std::optional<ItemId> head;

    std::optional<ItemId>& slot(EquipSlot s);
    const std::optional<ItemId>& slot(EquipSlot s) const;
    bool isEquipped(const ItemId& id) const;
};

struct EnemyInstance {
    EnemyId def;
    int hp = 0;
    Behavior behavior = Behavior::Aggressive;  // после пробуждения — Aggressive
    int wakeIn = -1;                           // −1 — спит спокойно; ≥ 0 — ходов до пробуждения
    int patternIndex = 0;
    int phase = 0;         // 0 — базовый паттерн, 1.. — фазы босса
    bool exposed = false;  // «Раскрыт» после Поглощения: урон ×2 в следующем раунде
    bool engaged = false;  // бой с ним уже начинался (паттерн не перезапускается)
};

struct ObjectState {
    bool opened = false;
    std::vector<ItemId> contents;
    int usesLeft = -1;
    std::set<int> doneInteractions;
};

struct RoomState {
    bool visited = false;
    bool lit = false;
    std::vector<ItemId> items;
    std::vector<EnemyInstance> enemies;
    std::set<Direction> revealedExits;
    std::set<Direction> unlockedExits;
    std::map<ObjectId, ObjectState> objects;
};

struct PlayerState {
    RoomId location;
    RoomId previousLocation;
    int hp = 0;
    int maxHp = 0;
    int baseAtk = 0;
    int oil = 0;
    int maxOil = 0;
    bool lanternLit = true;
    int darknessCounter = 0;
    Inventory inventory;
    Equipment equipment;
    std::set<ItemId> upgrades;
    std::set<ItemId> readNotes;
};

// Текущий бой. Существует только во время боя и не сохраняется.
struct Encounter {
    RoomId room;
    std::size_t enemyIndex = 0;
    IntentType intent = IntentType::Attack;
    bool counterReady = false;
    int round = 0;
};

struct Stats {
    int turns = 0;
    int oilBurned = 0;
    int deaths = 0;
    int kills = 0;
};

class WorldState {
public:
    PlayerState player;
    std::unordered_map<RoomId, RoomState> rooms;
    std::set<FlagId> flags;
    Stats stats;
    std::set<HintId> shownHints;
    std::optional<Encounter> encounter;

    RoomState& room(const RoomId& id) { return rooms.at(id); }
    const RoomState& room(const RoomId& id) const { return rooms.at(id); }
    RoomState& currentRoom() { return rooms.at(player.location); }
    const RoomState& currentRoom() const { return rooms.at(player.location); }
    bool hasFlag(const FlagId& f) const { return flags.count(f) > 0; }
    void setFlag(const FlagId& f) { flags.insert(f); }
    void clearFlag(const FlagId& f) { flags.erase(f); }
};

}  // namespace ll
