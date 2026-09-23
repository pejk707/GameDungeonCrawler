#include "systems/HintSystem.h"

#include "core/GameContext.h"

namespace ll {

void HintSystem::trigger(const HintId& id, GameContext& ctx) const {
    if (!ctx.settings.hints || ctx.world.shownHints.count(id)) return;
    const std::string key = "hint." + id;
    if (!ctx.data.strings.has(key)) return;
    ctx.world.shownHints.insert(id);
    ctx.say(MsgType::Hint, ctx.fmt("hint.format", {{"text", ctx.str(key)}}));
}

void HintSystem::onTurn(GameContext& ctx) const {
    const PlayerState& p = ctx.world.player;
    if (p.oil > 0 && p.oil <= 25) trigger("low_oil", ctx);
}

}  // namespace ll
