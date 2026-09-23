#include "commands/HandlerBase.h"

#include "core/GameContext.h"
#include "core/Systems.h"

namespace ll {

bool HandlerBase::canSee(const GameContext& ctx) { return ctx.sys.light.canSee(ctx); }

std::optional<EntityRef> HandlerBase::find(const std::vector<std::string>& words, unsigned scopeMask,
                                           GameContext& ctx, ActionResult& result, bool isTarget,
                                           const char* whatKey) const {
    if (words.empty()) {
        ctx.sayKey(MsgType::System, whatKey);
        return std::nullopt;
    }
    const bool see = canSee(ctx);
    const Resolution r = resolver_.resolve(words, scopeMask, ctx, see);
    if (r.found()) return r.ref;
    if (r.kind == Resolution::Kind::Ambiguous) {
        std::string options;
        for (std::size_t i = 0; i < r.candidates.size(); ++i) {
            if (i > 0) options += (i + 1 == r.candidates.size()) ? ctx.str("parse.or") : ", ";
            options += EntityResolver::name(r.candidates[i], ctx);
        }
        ctx.sayFmt(MsgType::System, "parse.ambiguous", {{"options", options}});
        result.needsClarification = true;
        result.clarifyTarget = isTarget;
        return std::nullopt;
    }
    std::string phrase;
    for (const auto& w : words) phrase += (phrase.empty() ? "" : " ") + w;
    if (!see && (scopeMask & (scope::Floor | scope::Objects))) {
        ctx.sayFmt(MsgType::Dark, "parse.not_found_dark", {{"word", phrase}});
    } else {
        ctx.sayFmt(MsgType::System, "parse.not_found", {{"word", phrase}});
    }
    return std::nullopt;
}

}  // namespace ll
