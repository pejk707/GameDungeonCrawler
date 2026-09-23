#include "GameHarness.h"
#include "systems/DamageCalculator.h"

using namespace ll;
using test::contains;

TEST_CASE("формулы: удар оружием") {
    const CombatConfig cfg;
    const DamageCalculator calc(cfg);
    FixedRandom rng;  // range → нижняя граница, chance → false (нет крита, нет промаха)

    WeaponHitInput in;
    in.attack = 5;
    in.targetDef = 2;
    CHECK(calc.weaponHit(in, rng).damage == 3);  // 5 + 0 − 2

    rng.pushRange(2);
    CHECK(calc.weaponHit(in, rng).damage == 5);  // 5 + 2 − 2

    in.guarding = true;  // ЗАЩИТА: ceil(3 × 0.5) = 2
    CHECK(calc.weaponHit(in, rng).damage == 2);
    in.guarding = false;

    in.overheated = true;  // ПЕРЕГРЕВ: защита не учитывается
    CHECK(calc.weaponHit(in, rng).damage == 5);
    in.overheated = false;

    in.counter = true;  // контрудар ×1.5
    CHECK(calc.weaponHit(in, rng).damage == 4);
    in.counter = false;

    in.doubleDamage = true;  // «Раскрыт» ×2
    CHECK(calc.weaponHit(in, rng).damage == 6);
    in.doubleDamage = false;

    rng.pushChance(true);  // крит ×2
    const HitResult crit = calc.weaponHit(in, rng);
    CHECK(crit.crit);
    CHECK(crit.damage == 6);

    in.targetDef = 20;  // минимум 1
    CHECK(calc.weaponHit(in, rng).damage == 1);

    in.inDarkness = true;  // в темноте — шанс промаха
    rng.pushChance(true);
    CHECK(calc.weaponHit(in, rng).darkMiss);
}

TEST_CASE("формулы: вспышка, удар врага, блок") {
    const CombatConfig cfg;
    const DamageCalculator calc(cfg);
    FixedRandom rng;
    CHECK(calc.flare(6, true, false) == 9);    // светобоязнь ×1.5
    CHECK(calc.flare(13, false, true) == 26);  // «Раскрыт» ×2
    CHECK(calc.enemyHit(6, 2, rng) == 4);
    CHECK(calc.enemyHit(2, 5, rng) == 1);  // минимум 1
    CHECK(calc.blocked(5, IntentType::Attack) == 1);
    CHECK(calc.blocked(10, IntentType::Heavy) == 0);
}

namespace {

// Начать бой с врагом в комнате пролога и дождаться режима боя.
void startFight(test::GameHarness& h, const RoomId& room, const std::string& attackCmd) {
    h.cmd("1");
    h.teleport(room);
    h.cmd(attackCmd);
    REQUIRE(h.game().state() == StateId::Combat);
}

}  // namespace

TEST_CASE("бой: блок сильного удара даёт контрудар") {
    test::GameHarness h;
    startFight(h, "gate_hall", "атаковать крысу");
    const int hp = h.player().hp;
    h.world().encounter->intent = IntentType::Heavy;
    CHECK(contains(h.cmd("б"), "контрудар"));
    CHECK(h.player().hp == hp);
    CHECK(h.world().encounter->counterReady);
}

TEST_CASE("бой: «Задуть» съедает масло, а блок его отменяет") {
    test::GameHarness h;
    startFight(h, "gate_hall", "атаковать крысу");
    h.world().room("gate_hall").enemies[0].hp = 100;  // чтобы крыса не умерла раньше времени
    const int oil = h.player().oil;
    h.world().encounter->intent = IntentType::Snuff;
    h.cmd("блок");
    CHECK(h.player().oil == oil);
    h.world().encounter->intent = IntentType::Snuff;
    h.cmd("блок");
    h.world().encounter->intent = IntentType::Snuff;
    h.cmd("атака");
    CHECK(h.player().oil == oil - 10);
}

TEST_CASE("бой: «Поглощение» раскрывает врага, урон по нему удваивается") {
    test::GameHarness h;
    startFight(h, "gate_hall", "атаковать крысу");
    EnemyInstance& rat = h.world().room("gate_hall").enemies[0];
    rat.hp = 50;
    const int oil = h.player().oil;
    h.world().encounter->intent = IntentType::Drain;
    h.cmd("б");
    CHECK(h.player().oil == oil - 5);  // блок: 5 вместо 20
    CHECK(rat.exposed);
    h.world().encounter->intent = IntentType::Guard;
    CHECK(contains(h.cmd("а"), "двойной урон"));
    CHECK_FALSE(rat.exposed);  // «Раскрыт» длится ровно один раунд
}

TEST_CASE("бой: вспышка оглушает светобоязненных и кормит Глотателя") {
    test::GameHarness h;
    h.cmd("1");
    h.teleport("great_stair");
    h.cmd("атаковать сажевика");
    REQUIRE(h.game().state() == StateId::Combat);
    const int hp = h.player().hp;
    h.world().room("great_stair").enemies[0].hp = 100;
    h.world().encounter->intent = IntentType::Attack;
    const std::string out = h.cmd("в");
    CHECK(contains(out, "теряет ход"));
    CHECK(h.player().hp == hp);                                 // оглушённый сажевик не ударил
    CHECK(h.world().room("great_stair").enemies[0].hp == 91);  // 6 × 1.5 = 9

    test::GameHarness h2;
    h2.cmd("1");
    h2.ctx().sys.combat.spawnAndBegin("lighteater", h2.ctx());
    h2.cmd("о");
    REQUIRE(h2.game().state() == StateId::Combat);
    EnemyInstance& eater = h2.world().currentRoom().enemies.back();
    eater.hp = 80;
    h2.world().encounter->intent = IntentType::Guard;
    CHECK(contains(h2.cmd("вспышка"), "глотает свет"));
    CHECK(eater.hp == 86);
    CHECK(contains(h2.cmd("бежать"), "Бежать некуда"));
}

TEST_CASE("бой: фаза босса на половине здоровья") {
    test::GameHarness h;
    h.cmd("1");
    h.ctx().sys.combat.spawnAndBegin("lighteater", h.ctx());
    h.cmd("о");
    EnemyInstance& eater = h.world().currentRoom().enemies.back();
    eater.hp = 51;
    h.world().encounter->intent = IntentType::Guard;
    h.player().equipment.weapon = "forge_hammer";
    h.player().inventory.add("forge_hammer");
    CHECK(contains(h.cmd("а"), "тьма в зале сгущается"));
    CHECK(eater.phase == 1);
    CHECK(h.world().encounter->intent == IntentType::Heavy);  // новый паттерн с первого намерения
}

TEST_CASE("бой: победа — лут на полу, флаг, возврат к исследованию") {
    test::GameHarness h;
    startFight(h, "gate_hall", "атаковать крысу");
    h.world().room("gate_hall").enemies[0].hp = 1;
    h.world().encounter->intent = IntentType::Attack;
    h.cmd("а");
    CHECK(h.game().state() == StateId::Exploration);
    CHECK(h.world().room("gate_hall").enemies.empty());
    CHECK(h.world().hasFlag("defeated_rat"));
    CHECK(h.world().stats.kills == 1);
}

TEST_CASE("бой: смерть игрока ведёт на экран поражения") {
    test::GameHarness h;
    startFight(h, "gate_hall", "атаковать крысу");
    h.world().room("gate_hall").enemies[0].hp = 100;
    h.player().hp = 1;
    h.world().encounter->intent = IntentType::Heavy;
    const std::string out = h.cmd("а");
    CHECK(h.game().state() == StateId::GameOver);
    CHECK(contains(out, "оказывается сильнее"));
    CHECK(h.world().stats.deaths == 1);
    h.cmd("2");  // новая игра
    CHECK(h.game().state() == StateId::Exploration);
    CHECK(h.player().hp == h.player().maxHp);
}

TEST_CASE("бой: внезапная атака по спящему врагу") {
    test::GameHarness h;
    h.cmd("1");
    h.teleport("quarters");
    EnemyInstance sleeper;
    sleeper.def = "drowned";
    sleeper.hp = 16;
    sleeper.behavior = Behavior::Sleeping;
    h.world().room("quarters").enemies.push_back(sleeper);
    const std::string out = h.cmd("атаковать утопленника");
    CHECK(contains(out, "Внезапная атака"));
    CHECK(h.world().room("quarters").enemies[0].hp < 16);
    CHECK(h.game().state() == StateId::Combat);
}

TEST_CASE("бой: агрессивный враг нападает при входе в комнату") {
    test::GameHarness h;
    h.cmd("1");
    h.player().inventory.add("gate_key");
    h.cmd("с");
    const std::string out = h.cmd("с");
    CHECK(contains(out, "бросается крыса"));
    CHECK(h.game().state() == StateId::Combat);
    CHECK(contains(out, "Намерение"));
}
