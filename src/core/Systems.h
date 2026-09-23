#pragma once

#include "core/TurnSystem.h"
#include "systems/CombatSystem.h"
#include "systems/HintSystem.h"
#include "systems/InteractionSystem.h"
#include "systems/LightSystem.h"
#include "systems/MovementSystem.h"
#include "systems/SaveSystem.h"
#include "systems/ScoreSystem.h"

namespace ll {

// Все игровые системы. Состояния они не хранят: всё изменяемое лежит в WorldState.
struct Systems {
    LightSystem light;
    MovementSystem movement;
    InteractionSystem interaction;
    CombatSystem combat;
    SaveSystem save;
    ScoreSystem score;
    HintSystem hints;
    TurnSystem turn;
};

}  // namespace ll
