#pragma once

#include <cstddef>

#include "common/Enums.h"
#include "common/Ids.h"

namespace ll {

struct GameContext;
struct ExitDef;

struct MoveResult {
    bool moved = false;
    bool tookTurn = false;
    bool newZone = false;  // первый вход в зону — показать баннер
};

// Перемещение между комнатами: выходы, замки, стоимость в масле, враги при входе.
class MovementSystem {
public:
    MoveResult move(Direction dir, GameContext& ctx) const;
    void arrive(GameContext& ctx) const;  // после описания комнаты: враги и подсказки
    bool engageAggressive(GameContext& ctx) const;  // начать бой с бодрствующим врагом, если он есть
    bool retreat(GameContext& ctx) const;  // бегство из боя в предыдущую комнату
    void wakeEnemy(std::size_t index, GameContext& ctx) const;

    const ExitDef* findExit(const RoomId& room, Direction dir, const GameContext& ctx) const;
    bool isLocked(const RoomId& room, const ExitDef& exit, const GameContext& ctx) const;

private:
    void enter(const RoomId& target, GameContext& ctx) const;
};

}  // namespace ll
