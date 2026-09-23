#include <fstream>
#include <sstream>

#include "GameHarness.h"
#include "systems/CombatSystem.h"

using namespace ll;

namespace {

bool has(test::GameHarness& h, const ItemId& id) { return h.player().inventory.has(id); }

// Между командами: долить масло и перевязаться, если нужно.
void maintain(test::GameHarness& h) {
    if (h.game().state() != StateId::Exploration) return;
    PlayerState& p = h.player();
    if (p.oil < 45 && has(h, "oil_flask")) h.cmd("использовать флакон");
    if (p.oil < 45 && has(h, "oil_can")) h.cmd("использовать канистру");
    if (p.hp < p.maxHp / 2 && has(h, "bandage")) h.cmd("использовать бинт");
}

// Простая тактика: отвечаем на объявленное намерение врага.
std::string chooseAction(test::GameHarness& h) {
    GameContext& ctx = h.ctx();
    const PlayerState& p = h.player();
    EnemyInstance* e = ctx.sys.combat.currentEnemy(ctx);
    const EnemyDef& def = ctx.data.enemy(e->def);
    const IntentType intent = ctx.world.encounter->intent;
    const bool lowHp = p.hp * 100 < p.maxHp * 35;

    if (lowHp && has(h, "tonic")) return "использовать настой";
    if (lowHp && has(h, "bandage")) return "использовать бинт";
    if (def.has(Trait::LightEater) && e->exposed && p.oil >= 8) return "вспышка";
    switch (intent) {
        case IntentType::Heavy:
        case IntentType::Snuff:
        case IntentType::Drain:
            return "блок";
        case IntentType::Guard:
            if (def.has(Trait::Photophobic) && p.oil >= 20) return "вспышка";
            if (def.has(Trait::Boss) && has(h, "sparkbomb")) return "использовать искромёт";
            return "атака";
        case IntentType::Overheat:
            return "атака";
        case IntentType::Attack:
            if (def.has(Trait::Photophobic) && !def.has(Trait::Boss) && p.oil >= 30) return "вспышка";
            return "атака";
    }
    return "атака";
}

void fight(test::GameHarness& h, int lineNo) {
    for (int round = 0; round < 200 && h.game().state() == StateId::Combat; ++round) h.cmd(chooseAction(h));
    INFO("строка сценария " << lineNo);
    REQUIRE(h.game().state() != StateId::Combat);
}

}  // namespace

TEST_CASE("сценарий: игру можно пройти от начала до конца") {
    test::GameHarness h(2024);
    std::ifstream in(std::string(LAMPLIGHTER_SOURCE_DIR) + "/tests/walkthrough.txt", std::ios::binary);
    REQUIRE(in);
    std::string line;
    int lineNo = 0;
    std::string last;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        maintain(h);
        INFO("строка " << lineNo << ": «" << line << "»\n" << last);
        REQUIRE(h.game().state() != StateId::GameOver);
        last = h.cmd(line);
        if (h.game().state() == StateId::Combat) fight(h, lineNo);
        REQUIRE(h.game().state() != StateId::GameOver);
    }
    CHECK(h.game().state() == StateId::Victory);
    const WorldState& w = h.world();
    MESSAGE("ходов: " << w.stats.turns << ", масла сожжено: " << w.stats.oilBurned << ", масла осталось: "
                      << w.player.oil << ", HP: " << w.player.hp << "/" << w.player.maxHp << ", побед: "
                      << w.stats.kills << ", записок: " << w.player.readNotes.size());
    CHECK(w.stats.kills == 17);  // 13 обычных врагов и 4 босса
    CHECK(w.player.readNotes.size() == 8);
    CHECK(h.ctx().sys.score.trueEnding(h.ctx()));
}
