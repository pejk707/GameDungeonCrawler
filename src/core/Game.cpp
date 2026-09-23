#include "core/Game.h"

#include <chrono>

#include "core/states/CombatState.h"
#include "core/states/ExplorationState.h"
#include "core/states/GameOverState.h"
#include "core/states/MainMenuState.h"
#include "core/states/VictoryState.h"
#include "data/DataLoader.h"
#include "data/DataValidator.h"
#include "model/WorldFactory.h"

namespace ll {
namespace {

std::uint32_t timeSeed() {
    return static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
}

}  // namespace

Game::Game(Options options, std::unique_ptr<IConsole> console)
    : options_(std::move(options)),
      console_(std::move(console)),
      rng_(options_.seed.value_or(timeSeed())),
      renderer_(console_ && console_->supportsColor() && !options_.noColor, 78) {}

bool Game::init() {
    try {
        data_ = DataLoader(options_.root).loadAll();
    } catch (const DataError& e) {
        log_.push(MsgType::Damage, std::string("Ошибка загрузки данных: ") + e.what());
        return false;
    }
    const auto errors = DataValidator().validate(data_);
    if (!errors.empty()) {
        log_.push(MsgType::Damage, "Ошибки в данных игры:");
        for (const auto& e : errors) log_.push(MsgType::Damage, "  " + e);
        return false;
    }
    renderer_ = Renderer(renderer_.color(), static_cast<std::size_t>(options_.width > 0 ? options_.width
                                                                                        : data_.config.width));
    settings_.savePath = (options_.root / "saves" / "save.json").string();
    world_ = WorldFactory::create(data_);
    ctx_ = std::make_unique<GameContext>(data_, world_, log_, rng_, systems_, settings_);

    fsm_.add(StateId::MainMenu, std::make_unique<MainMenuState>());
    fsm_.add(StateId::Exploration, std::make_unique<ExplorationState>());
    fsm_.add(StateId::Combat, std::make_unique<CombatState>());
    fsm_.add(StateId::GameOver, std::make_unique<GameOverState>());
    fsm_.add(StateId::Victory, std::make_unique<VictoryState>());
    return true;
}

void Game::start() { fsm_.change(StateId::MainMenu, *ctx_); }

void Game::settle(Transition t) {
    if (t.kind == TransitionKind::Quit) {
        running_ = false;
        return;
    }
    if (auto pending = ctx_->takePending()) {
        fsm_.change(*pending, *ctx_);
    } else if (t.kind == TransitionKind::Change) {
        fsm_.change(t.target, *ctx_);
    }
    // onEnter нового режима тоже может запросить переход (например, следующий враг в комнате).
    for (int guard = 0; guard < 4; ++guard) {
        auto pending = ctx_->takePending();
        if (!pending) break;
        fsm_.change(*pending, *ctx_);
    }
}

bool Game::step(const std::string& line) {
    settle(fsm_.current().handleInput(*ctx_, line));
    return running_;
}

std::string Game::takeOutput() {
    std::string out = log_.joinedText();
    log_.drain();
    return out;
}

int Game::run() {
    if (!ctx_) {
        renderer_.flush(log_, *console_);
        return 1;
    }
    if (renderer_.color()) console_->write("\x1b]0;Последний фонарщик\x07");  // заголовок окна
    start();
    while (running_) {
        if (!log_.empty()) log_.blank();
        renderer_.flush(log_, *console_);
        console_->write(fsm_.current().prompt(*ctx_));
        const auto line = console_->readLine();
        if (!line) break;
        step(*line);
    }
    renderer_.flush(log_, *console_);
    return 0;
}

}  // namespace ll
