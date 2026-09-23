#include "common/Enums.h"

#include <utility>

namespace ll {
namespace {

template <class E, std::size_t N>
using Table = std::array<std::pair<E, std::string_view>, N>;

constexpr Table<Direction, 6> kDirections{{{Direction::North, "north"},
                                           {Direction::South, "south"},
                                           {Direction::East, "east"},
                                           {Direction::West, "west"},
                                           {Direction::Up, "up"},
                                           {Direction::Down, "down"}}};
constexpr Table<ItemType, 7> kItemTypes{{{ItemType::Weapon, "weapon"},
                                         {ItemType::Armor, "armor"},
                                         {ItemType::Consumable, "consumable"},
                                         {ItemType::Upgrade, "upgrade"},
                                         {ItemType::Key, "key"},
                                         {ItemType::Note, "note"},
                                         {ItemType::Lantern, "lantern"}}};
constexpr Table<EquipSlot, 3> kSlots{
    {{EquipSlot::Weapon, "weapon"}, {EquipSlot::Body, "body"}, {EquipSlot::Head, "head"}}};
constexpr Table<ObjectKind, 4> kObjectKinds{{{ObjectKind::Plain, "plain"},
                                             {ObjectKind::Container, "container"},
                                             {ObjectKind::Brazier, "brazier"},
                                             {ObjectKind::Readable, "readable"}}};
constexpr Table<Trigger, 4> kTriggers{{{Trigger::Use, "use"},
                                       {Trigger::Light, "light"},
                                       {Trigger::Open, "open"},
                                       {Trigger::Examine, "examine"}}};
constexpr Table<IntentType, 6> kIntents{{{IntentType::Attack, "attack"},
                                         {IntentType::Heavy, "heavy"},
                                         {IntentType::Guard, "guard"},
                                         {IntentType::Snuff, "snuff"},
                                         {IntentType::Overheat, "overheat"},
                                         {IntentType::Drain, "drain"}}};
constexpr Table<Behavior, 2> kBehaviors{
    {{Behavior::Aggressive, "aggressive"}, {Behavior::Sleeping, "sleeping"}}};
constexpr Table<Trait, 3> kTraits{{{Trait::Boss, "boss"},
                                   {Trait::Photophobic, "photophobic"},
                                   {Trait::LightEater, "light_eater"}}};

template <class E, std::size_t N>
std::string_view keyOf(const Table<E, N>& table, E value) {
    for (const auto& [v, name] : table) {
        if (v == value) return name;
    }
    return "?";
}

template <class E, std::size_t N>
std::optional<E> valueOf(const Table<E, N>& table, std::string_view key) {
    for (const auto& [v, name] : table) {
        if (name == key) return v;
    }
    return std::nullopt;
}

}  // namespace

std::string_view toKey(Direction v) { return keyOf(kDirections, v); }
std::string_view toKey(ItemType v) { return keyOf(kItemTypes, v); }
std::string_view toKey(EquipSlot v) { return keyOf(kSlots, v); }
std::string_view toKey(ObjectKind v) { return keyOf(kObjectKinds, v); }
std::string_view toKey(Trigger v) { return keyOf(kTriggers, v); }
std::string_view toKey(IntentType v) { return keyOf(kIntents, v); }
std::string_view toKey(Behavior v) { return keyOf(kBehaviors, v); }
std::string_view toKey(Trait v) { return keyOf(kTraits, v); }

template <>
std::optional<Direction> fromKey<Direction>(std::string_view key) {
    return valueOf(kDirections, key);
}
template <>
std::optional<ItemType> fromKey<ItemType>(std::string_view key) {
    return valueOf(kItemTypes, key);
}
template <>
std::optional<EquipSlot> fromKey<EquipSlot>(std::string_view key) {
    return valueOf(kSlots, key);
}
template <>
std::optional<ObjectKind> fromKey<ObjectKind>(std::string_view key) {
    return valueOf(kObjectKinds, key);
}
template <>
std::optional<Trigger> fromKey<Trigger>(std::string_view key) {
    return valueOf(kTriggers, key);
}
template <>
std::optional<IntentType> fromKey<IntentType>(std::string_view key) {
    return valueOf(kIntents, key);
}
template <>
std::optional<Behavior> fromKey<Behavior>(std::string_view key) {
    return valueOf(kBehaviors, key);
}
template <>
std::optional<Trait> fromKey<Trait>(std::string_view key) {
    return valueOf(kTraits, key);
}

Direction opposite(Direction d) {
    switch (d) {
        case Direction::North: return Direction::South;
        case Direction::South: return Direction::North;
        case Direction::East: return Direction::West;
        case Direction::West: return Direction::East;
        case Direction::Up: return Direction::Down;
        case Direction::Down: return Direction::Up;
    }
    return d;
}

}  // namespace ll
