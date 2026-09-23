#include "GameHarness.h"
#include "TestSupport.h"
#include "data/DataLoader.h"
#include "data/SaveCodec.h"
#include "model/WorldFactory.h"
#include "ui/MapRenderer.h"

using namespace ll;
using test::contains;

TEST_CASE("сохранение: save → load возвращает то же состояние") {
    const GameData& data = test::gameData();
    WorldState w = WorldFactory::create(data);
    w.player.location = "gatehouse";
    w.player.hp = 17;
    w.player.oil = 42;
    w.player.inventory.add("gate_key");
    w.player.inventory.add("oil_flask", 2);
    w.player.equipment.weapon = "wick_knife";
    w.player.readNotes.insert("note_orin");
    w.flags.insert("pump_fixed");
    w.room("gatehouse").lit = true;
    w.room("gatehouse").objects.at("desk").opened = true;
    w.room("gatehouse").objects.at("desk").contents.clear();
    w.room("gatehouse").unlockedExits.insert(Direction::North);
    w.room("gate_hall").enemies[0].hp = 2;
    w.stats.turns = 42;

    const WorldState r = SaveCodec::fromJson(SaveCodec::toJson(w), data);
    CHECK(r.player.location == "gatehouse");
    CHECK(r.player.hp == 17);
    CHECK(r.player.oil == 42);
    CHECK(r.player.inventory.count("oil_flask") == 2);
    CHECK(r.player.equipment.weapon == std::optional<ItemId>("wick_knife"));
    CHECK(r.player.readNotes.count("note_orin") == 1);
    CHECK(r.hasFlag("pump_fixed"));
    CHECK(r.room("gatehouse").lit);
    CHECK(r.room("gatehouse").objects.at("desk").opened);
    CHECK(r.room("gatehouse").objects.at("desk").contents.empty());
    CHECK(r.room("gatehouse").unlockedExits.count(Direction::North) == 1);
    CHECK(r.room("gate_hall").enemies[0].hp == 2);
    CHECK(r.stats.turns == 42);
    CHECK_FALSE(r.encounter.has_value());
}

TEST_CASE("сохранение: битые данные дают понятную ошибку") {
    const GameData& data = test::gameData();
    auto j = SaveCodec::toJson(WorldFactory::create(data));
    j["player"]["location"] = "atlantis";
    CHECK_THROWS_AS(SaveCodec::fromJson(j, data), DataError);
    auto k = SaveCodec::toJson(WorldFactory::create(data));
    k["player"]["inventory"].push_back({"golden_crown", 1});
    CHECK_THROWS_AS(SaveCodec::fromJson(k, data), DataError);
}

TEST_CASE("сохранение у жаровни и загрузка после смерти") {
    test::GameHarness h;
    h.cmd("1");
    h.cmd("с");
    CHECK(contains(h.cmd("сохранить"), "только у горящей жаровни"));
    h.cmd("зажечь камин");
    CHECK(contains(h.cmd("отдохнуть"), "сохранена"));
    const int oil = h.player().oil;

    h.player().oil = 3;  // «испортим» мир…
    h.cmd("загрузить");  // …и вернёмся к огню
    CHECK(h.player().oil == oil);
    CHECK(h.player().location == "gatehouse");

    h.cmd("выход");
    CHECK(h.game().state() == StateId::MainMenu);
    h.cmd("2");  // «Продолжить»
    CHECK(h.game().state() == StateId::Exploration);
    CHECK(h.player().location == "gatehouse");
}

TEST_CASE("карта показывает посещённые комнаты и игрока") {
    test::GameHarness h;
    h.cmd("1");
    h.cmd("с");
    const std::string map = MapRenderer::render(h.ctx());
    CHECK(contains(map, "[Сторожка]@"));
    CHECK(contains(map, "[Тропа"));
    CHECK(contains(map, "│"));
    CHECK(contains(h.cmd("карта"), "Ворота Горна"));
}
