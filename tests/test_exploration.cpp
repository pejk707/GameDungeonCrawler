#include "GameHarness.h"

using namespace ll;
using test::contains;

TEST_CASE("пролог: сторожка, ключ, камин, дверь") {
    test::GameHarness h;
    const std::string intro = h.cmd("1");
    CHECK(h.game().state() == StateId::Exploration);
    CHECK(contains(intro, "Горная тропа"));

    h.cmd("с");
    CHECK(h.player().location == "gatehouse");
    CHECK(h.player().oil == 78);  // сторожка тёмная: переход стоит 2 масла

    CHECK(contains(h.cmd("с"), "заперта"));
    CHECK(h.player().location == "gatehouse");

    CHECK(contains(h.cmd("взять ключ"), "нет ничего похожего"));  // ключ в закрытом ящике
    CHECK(contains(h.cmd("открыть стол"), "ключ от ворот"));
    h.cmd("взять ключ");
    CHECK(h.player().inventory.has("gate_key"));

    CHECK(contains(h.cmd("отдохнуть"), "только у горящей жаровни"));
    h.cmd("зажечь камин");
    CHECK(h.world().room("gatehouse").lit);
    CHECK(h.player().oil == 68);
    CHECK(contains(h.cmd("отдохнуть"), "отдыхаете"));

    CHECK(contains(h.cmd("с"), "отпираете"));
    CHECK(h.player().location == "gate_hall");
}

TEST_CASE("предметы: взять всё, надеть нож, прочитать письмо") {
    test::GameHarness h;
    h.cmd("1");
    h.cmd("с");
    h.cmd("взять всё");
    CHECK(h.player().inventory.has("wick_knife"));
    CHECK(h.player().inventory.has("note_orin"));
    CHECK(h.player().inventory.count("oil_flask") == 1);

    CHECK(contains(h.cmd("надеть нож"), "Атака: 3"));
    CHECK(h.player().equipment.weapon == std::optional<ItemId>("wick_knife"));

    CHECK(contains(h.cmd("читать письмо"), "Печать Сердца"));
    CHECK(h.player().readNotes.count("note_orin") == 1);

    CHECK(contains(h.cmd("выпить флакон"), "Фонарь и так полон") == false);
    CHECK(h.player().oil == 100);  // 78 + 30, но не больше максимума
}

TEST_CASE("уточнение: какой ключ?") {
    test::GameHarness h;
    h.cmd("1");
    h.player().inventory.add("gate_key");
    h.player().inventory.add("rusty_key");
    CHECK(contains(h.cmd("осмотреть ключ"), "Какой именно"));
    CHECK(contains(h.cmd("ржавый"), "Склад кузни"));
}

TEST_CASE("тьма: без света смерть на четвёртом ходу") {
    test::GameHarness h;
    h.cmd("1");
    h.player().inventory.add("gate_key");
    h.world().room("gate_hall").enemies.clear();  // проверяем тьму, а не бой
    h.world().room("great_stair").enemies.clear();
    h.cmd("с");
    h.cmd("с");  // привратный зал, тёмный
    REQUIRE(h.player().location == "gate_hall");
    h.cmd("потушить фонарь");
    CHECK_FALSE(h.ctx().sys.light.canSee(h.ctx()));
    CHECK(contains(h.cmd("вн"), "съест что-то голодное"));
    CHECK(h.player().darknessCounter == 1);
    h.cmd("вв");
    h.cmd("вн");
    CHECK(h.player().darknessCounter == 3);
    h.cmd("фонарь");
    CHECK(h.player().darknessCounter == 0);  // свет сразу сбрасывает счётчик
}

TEST_CASE("цистерна в казармах: две заправки") {
    test::GameHarness h;
    h.cmd("1");
    h.player().location = "quarters";
    h.player().oil = 10;
    h.cmd("использовать цистерну");
    CHECK(h.player().oil == 50);
    h.cmd("использовать цистерну");
    CHECK(h.player().oil == 90);
    CHECK(contains(h.cmd("использовать цистерну"), "больше ничего не осталось"));
    CHECK(h.player().oil == 90);
}

TEST_CASE("непонятные команды не тратят ход") {
    test::GameHarness h;
    h.cmd("1");
    const int turns = h.world().stats.turns;
    CHECK(contains(h.cmd("танцевать"), "Не понимаю"));
    CHECK(contains(h.cmd("з"), "пути нет"));
    CHECK(contains(h.cmd("съесть фонарь"), "не настолько голодны"));
    CHECK(h.world().stats.turns == turns);
}
