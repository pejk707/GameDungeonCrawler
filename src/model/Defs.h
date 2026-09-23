#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "common/Enums.h"
#include "common/Ids.h"
#include "model/Interaction.h"

namespace ll {

// Определения — неизменяемый контент игры, загруженный из data/*.json.

struct MapPos {
    int x = 0;
    int y = 0;
};

struct LockDef {
    std::vector<Condition> conditions;
    std::string lockedMessage;
    std::string unlockMessage;
};

struct ExitDef {
    Direction dir = Direction::North;
    RoomId to;
    bool hidden = false;
    std::optional<LockDef> lock;
};

struct EnemyPlacement {
    EnemyId enemy;
    Behavior behavior = Behavior::Aggressive;
};

struct ObjectDef {
    ObjectId id;
    std::string name;
    std::vector<std::string> stems;
    ObjectKind kind = ObjectKind::Plain;
    std::string description;
    std::string text;  // для Readable: что написано
    std::vector<ItemId> contents;
    int uses = -1;  // −1 — без ограничения
    std::vector<InteractionDef> interactions;
};

struct RoomDef {
    RoomId id;
    std::string name;
    std::string mapLabel;
    ZoneId zone;
    MapPos map;
    bool litByDefault = false;
    std::string description;
    std::string descriptionDark;
    std::map<Direction, ExitDef> exits;
    std::vector<ItemId> items;
    std::vector<ObjectDef> objects;
    std::vector<EnemyPlacement> enemies;

    const ObjectDef* findObject(const ObjectId& objectId) const;
    const ObjectDef* brazier() const;
};

struct ItemEffect {
    int heal = 0;
    int oil = 0;
    int damage = 0;
    int maxHp = 0;
    int maxOil = 0;
    bool escape = false;
};

struct ItemDef {
    ItemId id;
    std::string name;
    std::vector<std::string> stems;
    ItemType type = ItemType::Consumable;
    std::optional<EquipSlot> slot;
    int atkBonus = 0;
    int defBonus = 0;
    int flareBonus = 0;
    ItemEffect effect;
    bool droppable = true;
    bool combatOnly = false;
    std::string description;
    std::string textFile;    // для записок
    std::string useMessage;  // текст при использовании
};

struct PhaseDef {
    double hpBelow = 0.5;
    std::vector<IntentType> pattern;
    std::string message;
};

struct EnemyDef {
    EnemyId id;
    std::string name;
    std::vector<std::string> stems;
    std::string description;
    int hp = 1;
    int atk = 1;
    int def = 0;
    std::set<Trait> traits;
    double dodgeChance = 0.0;
    int snuffAmount = 10;
    int drainAmount = 20;
    int drainBlocked = 5;
    std::vector<IntentType> pattern;
    std::vector<PhaseDef> phases;
    std::map<IntentType, std::string> intentText;
    std::vector<ItemId> loot;
    std::string introText;
    std::string defeatText;
    std::string sleepText;  // звук спящего врага в темноте
    std::string art;

    bool has(Trait t) const { return traits.count(t) > 0; }
};

struct PlayerStart {
    RoomId room;
    int hp = 30;
    int atk = 1;
    int oil = 80;
    int oilMax = 100;
    std::vector<ItemId> inventory;
    std::vector<ItemId> equipped;
};

struct OilCosts {
    int moveDark = 2;
    int flare = 8;
    int brazier = 10;
};

struct CombatConfig {
    double critChance = 0.10;
    double critMult = 2.0;
    double blockMult = 0.25;
    double counterMult = 1.5;
    double guardMult = 0.5;
    double fleeChance = 0.6;
    double darkMissChance = 0.5;
    double photophobiaMult = 1.5;
    int flareDamage = 6;
};

struct ScoreConfig {
    int base = 1000;
    int perOil = 5;
    int perNote = 50;
    int perTurn = 1;
    int perDeath = 100;
    int master = 1200;
    int lamplighter = 900;
};

struct GameTexts {
    std::string title;
    std::string victory;
    std::string death;
    std::string intro;
    std::string epilogueA;
    std::string epilogueB;
};

struct GameConfig {
    PlayerStart player;
    OilCosts oilCost;
    CombatConfig combat;
    ScoreConfig score;
    GameTexts texts;
    int darknessTurnsToDeath = 4;
    std::vector<int> oilWarnings{25, 10};
    std::vector<ItemId> embers;
    int totalNotes = 8;
    int width = 78;
};

}  // namespace ll
