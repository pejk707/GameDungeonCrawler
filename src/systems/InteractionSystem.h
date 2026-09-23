#pragma once

#include <optional>
#include <vector>

#include "common/Enums.h"
#include "common/Ids.h"
#include "model/Interaction.h"

namespace ll {

struct GameContext;

enum class InteractionOutcome {
    NotApplicable,  // нет подходящего взаимодействия — ход не тратится
    Refused,        // не выполнено требование (нет масла, фонарь погас) — ход не тратится
    Success,
    Failed          // условия загадки ложны, сработал onFail
};

// Интерпретатор взаимодействий из данных: все загадки игры проходят через него.
class InteractionSystem {
public:
    InteractionOutcome use(const std::optional<ItemId>& item, const ObjectId& object, GameContext& ctx) const;
    InteractionOutcome light(const ObjectId& object, GameContext& ctx) const;
    InteractionOutcome open(const ObjectId& object, GameContext& ctx) const;
    void examine(const ObjectId& object, GameContext& ctx) const;

    bool evaluate(const Condition& condition, const GameContext& ctx) const;
    bool allMet(const std::vector<Condition>& conditions, const GameContext& ctx) const;
    void apply(const Effect& effect, GameContext& ctx) const;

private:
    InteractionOutcome run(Trigger trigger, const std::optional<ItemId>& item, const ObjectId& object,
                           GameContext& ctx) const;
    InteractionOutcome lightBrazier(const ObjectId& object, GameContext& ctx) const;
    void listContents(const ObjectId& object, GameContext& ctx) const;
};

}  // namespace ll
