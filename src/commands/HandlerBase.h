#pragma once

#include <optional>
#include <string>
#include <vector>

#include "commands/ICommandHandler.h"
#include "parser/EntityResolver.h"

namespace ll {

// Общая часть обработчиков: поиск объекта с понятными сообщениями и уточнением.
class HandlerBase : public ICommandHandler {
protected:
    std::optional<EntityRef> find(const std::vector<std::string>& words, unsigned scopeMask, GameContext& ctx,
                                  ActionResult& result, bool isTarget = false,
                                  const char* whatKey = "parse.what") const;
    static bool canSee(const GameContext& ctx);

    EntityResolver resolver_;
};

}  // namespace ll
