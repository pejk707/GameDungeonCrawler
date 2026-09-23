#include "core/states/CombatState.h"

#include "core/GameContext.h"
#include "core/Systems.h"
#include "ui/RoomDescriber.h"

namespace ll {

void CombatState::onEnter(GameContext& ctx) {
    ctx.sys.combat.showHeader(ctx);
    ctx.sys.combat.announceIntent(ctx);
    ctx.sys.hints.trigger("combat", ctx);
    ctx.sayKey(MsgType::System, "combat.commands");
}

std::string CombatState::prompt(const GameContext& ctx) const { return ctx.str("combat.prompt"); }

Transition CombatState::handleInput(GameContext& ctx, const std::string& line) {
    const ParseResult parsed = parser_.parse(line, ctx.data.lexicon);
    if (!parsed.command) {
        ctx.sayKey(MsgType::System, "combat.commands");
        return Transition::none();
    }
    const ParsedCommand& cmd = *parsed.command;
    CombatAction action;
    switch (cmd.verb) {
        case Verb::Attack: action.kind = CombatAction::Kind::Attack; break;
        case Verb::Block: action.kind = CombatAction::Kind::Block; break;
        case Verb::Flare: action.kind = CombatAction::Kind::Flare; break;
        case Verb::Flee: action.kind = CombatAction::Kind::Flee; break;
        case Verb::Use: {
            if (cmd.object.empty()) {
                ctx.sayKey(MsgType::System, "use.what");
                return Transition::none();
            }
            const Resolution r = resolver_.resolve(cmd.object, scope::Inventory, ctx, true);
            if (!r.found()) {
                ctx.sayKey(MsgType::System, r.kind == Resolution::Kind::Ambiguous ? "combat.item_ambiguous"
                                                                                  : "combat.no_item");
                return Transition::none();
            }
            action.kind = CombatAction::Kind::UseItem;
            action.item = r.ref.id;
            break;
        }
        case Verb::Examine:
            ctx.sys.combat.examineEnemy(ctx);
            return Transition::none();
        case Verb::Status:
            ctx.sys.combat.showStatus(ctx);
            return Transition::none();
        case Verb::Inventory: {
            std::string list;
            for (const auto& [id, n] : ctx.world.player.inventory.entries()) {
                if (ctx.data.item(id).type != ItemType::Consumable) continue;
                if (!list.empty()) list += ", ";
                list += ctx.data.item(id).name + (n > 1 ? " ×" + std::to_string(n) : "");
            }
            ctx.sayFmt(MsgType::Item, "combat.consumables", {{"items", list.empty() ? ctx.str("combat.nothing") : list}});
            return Transition::none();
        }
        case Verb::Help:
            ctx.sayKey(MsgType::Text, "help.combat");
            return Transition::none();
        default:
            ctx.sayKey(MsgType::System, "combat.commands");
            return Transition::none();
    }

    const RoundOutcome outcome = ctx.sys.combat.playerAction(action, ctx);
    if (outcome == RoundOutcome::Continue) {
        ctx.sys.combat.showStatus(ctx);
        ctx.sys.combat.announceIntent(ctx);
    }
    if (auto next = ctx.takePending()) return Transition::to(*next);
    return Transition::none();
}

}  // namespace ll
