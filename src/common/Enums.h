#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace ll {

enum class Direction { North, South, East, West, Up, Down };
inline constexpr std::array<Direction, 6> kAllDirections{Direction::North, Direction::South,
                                                          Direction::East,  Direction::West,
                                                          Direction::Up,    Direction::Down};

enum class ItemType { Weapon, Armor, Consumable, Upgrade, Key, Note, Lantern };
enum class EquipSlot { Weapon, Body, Head };
enum class ObjectKind { Plain, Container, Brazier, Readable };
enum class Trigger { Use, Light, Open, Examine };
enum class IntentType { Attack, Heavy, Guard, Snuff, Overheat, Drain };
enum class Behavior { Aggressive, Sleeping };
enum class Trait { Boss, Photophobic, LightEater };
enum class StateId { MainMenu, Exploration, Combat, GameOver, Victory };

// Тип сообщения определяет цвет и способ вывода (см. Renderer).
enum class MsgType {
    Text,     // основной текст
    Dark,     // описание в темноте
    System,   // системные сообщения и ошибки ввода
    Hint,     // контекстные подсказки
    Title,    // заголовки комнат и экранов
    Status,   // строка состояния (без переноса)
    Damage,   // урон, опасность
    Heal,     // лечение, успех
    Item,     // предметы
    Intent,   // намерение врага
    Lore,     // записки
    Oil,      // масло и свет
    Art,      // ASCII-арт (без переноса)
    Map       // ASCII-карта (без переноса)
};

// Названия значений в файлах данных ("north", "heavy", ...).
std::string_view toKey(Direction v);
std::string_view toKey(ItemType v);
std::string_view toKey(EquipSlot v);
std::string_view toKey(ObjectKind v);
std::string_view toKey(Trigger v);
std::string_view toKey(IntentType v);
std::string_view toKey(Behavior v);
std::string_view toKey(Trait v);

// Разбор значения по его названию; std::nullopt — неизвестное название.
template <class E>
std::optional<E> fromKey(std::string_view key);

Direction opposite(Direction d);

}  // namespace ll
