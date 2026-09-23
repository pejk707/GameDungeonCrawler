#pragma once

#include "common/Ids.h"

namespace ll {

struct GameContext;

// Свет, масло и тьма — главная механика игры.
class LightSystem {
public:
    bool isRoomLit(const RoomId& room, const GameContext& ctx) const;
    bool canSee(const GameContext& ctx) const;          // комната освещена или горит фонарь
    bool lanternBurning(const GameContext& ctx) const;  // фонарь горит и в нём есть масло
    int moveCost(const RoomId& target, const GameContext& ctx) const;

    void spendOil(int amount, GameContext& ctx) const;  // списывает, предупреждает, гасит на нуле
    int addOil(int amount, GameContext& ctx) const;     // возвращает, сколько реально долито
    bool setLantern(bool on, GameContext& ctx) const;
    void tickDarkness(GameContext& ctx) const;          // вызывается в конце хода
};

}  // namespace ll
