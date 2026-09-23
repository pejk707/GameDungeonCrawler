#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "common/Enums.h"
#include "common/Ids.h"

namespace ll {

// Условия и эффекты взаимодействий: все загадки игры описаны этими данными,
// а исполняет их InteractionSystem.
struct HasFlag {
    FlagId flag;
};
struct NotFlag {
    FlagId flag;
};
struct HasItem {
    ItemId item;
};
struct EmberCount {
    int min = 0;
};
using Condition = std::variant<HasFlag, NotFlag, HasItem, EmberCount>;

struct SetFlag {
    FlagId flag;
};
struct ClearFlags {
    std::vector<FlagId> flags;
};
struct GiveItem {
    ItemId item;
    int count = 1;
};
struct ConsumeItem {
    ItemId item;
    int count = 1;
};
struct RevealExit {
    RoomId room;
    Direction dir = Direction::North;
};
struct UnlockExit {
    RoomId room;
    Direction dir = Direction::North;
};
struct SpawnEnemy {
    EnemyId enemy;
};
struct SpendOil {
    int amount = 0;
};
struct AddOil {
    int amount = 0;
};
struct LightRoom {};
struct ShowText {
    std::string text;
};
struct WinGame {};
using Effect = std::variant<SetFlag, ClearFlags, GiveItem, ConsumeItem, RevealExit, UnlockExit,
                            SpawnEnemy, SpendOil, AddOil, LightRoom, ShowText, WinGame>;

struct InteractionDef {
    Trigger trigger = Trigger::Use;
    std::optional<ItemId> useItem;       // для Trigger::Use: какой предмет применяют
    std::vector<Condition> conditions;   // логика загадки: если ложно — выполняется onFail
    std::vector<Effect> effects;
    std::vector<Effect> onFail;
    std::string message;
    std::string failMessage;
    bool once = false;
};

// Помощник для std::visit с набором лямбд.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace ll
