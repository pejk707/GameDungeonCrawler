#include "core/states/MainMenuState.h"

#include "common/Utf8.h"
#include "core/GameContext.h"
#include "parser/TextNormalizer.h"

namespace ll {

void MainMenuState::onEnter(GameContext& ctx) {
    ctx.say(MsgType::Art, ctx.data.texts.get(ctx.data.config.texts.title));
    showMenu(ctx);
}

void MainMenuState::showMenu(GameContext& ctx) const {
    ctx.log.blank();
    ctx.sayKey(MsgType::Title, "menu.options");
}

Transition MainMenuState::handleInput(GameContext& ctx, const std::string& line) {
    const auto tokens = TextNormalizer::tokenize(line);
    const std::string w = tokens.empty() ? std::string{} : tokens[0];
    auto is = [&](const char* digit, const char* prefix) {
        return w == digit || (!w.empty() && utf8::startsWith(w, prefix));
    };

    if (is("1", "нов") || w == "new") {
        ctx.newGame();
        ctx.log.blank();
        ctx.say(MsgType::Text, ctx.data.texts.get(ctx.data.config.texts.intro));
        ctx.log.blank();
        return Transition::to(StateId::Exploration);
    }
    if (is("2", "прод") || w == "continue") {
        ctx.sayKey(MsgType::System, "menu.no_save");
        showMenu(ctx);
        return Transition::none();
    }
    if (is("3", "об") || w == "about") {
        ctx.sayKey(MsgType::Text, "menu.about");
        showMenu(ctx);
        return Transition::none();
    }
    if (is("4", "вых") || w == "q" || w == "quit" || w == "exit") {
        ctx.sayKey(MsgType::System, "menu.bye");
        return Transition::quit();
    }
    ctx.sayKey(MsgType::System, "menu.unknown");
    showMenu(ctx);
    return Transition::none();
}

}  // namespace ll
