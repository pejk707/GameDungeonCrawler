#include <algorithm>

#include "TestSupport.h"
#include "data/DataValidator.h"
#include "doctest/doctest.h"
#include "model/WorldFactory.h"

using namespace ll;

TEST_CASE("данные игры загружаются и проходят проверку") {
    const GameData& data = test::gameData();
    const auto errors = DataValidator().validate(data);
    for (const auto& e : errors) MESSAGE(e);
    CHECK(errors.empty());
    CHECK(data.items.size() == 32);
    CHECK(data.enemies.size() == 10);
    CHECK(data.lexicon.verbCount(ParseMode::Exploration) > 20);
}

TEST_CASE("валидатор находит битые ссылки") {
    GameData data = test::gameData();
    data.rooms.at("gatehouse").exits.at(Direction::North).to = "nowhere";
    data.enemies.at("rat").loot.push_back("golden_crown");
    const auto errors = DataValidator().validate(data);
    auto mentions = [&](const std::string& s) {
        return std::any_of(errors.begin(), errors.end(),
                           [&](const std::string& e) { return e.find(s) != std::string::npos; });
    };
    CHECK(mentions("nowhere"));
    CHECK(mentions("golden_crown"));
}

TEST_CASE("DataLoader сообщает имя файла при ошибке") {
    CHECK_THROWS_AS(DataLoader("no/such/dir").loadAll(), DataError);
}

TEST_CASE("WorldFactory строит начальное состояние по определениям") {
    const GameData& data = test::gameData();
    const WorldState world = WorldFactory::create(data);
    CHECK(world.player.location == data.config.player.room);
    CHECK(world.player.hp == data.config.player.hp);
    CHECK(world.player.inventory.has("lantern"));
    CHECK(world.player.equipment.body == std::optional<ItemId>("leather_jacket"));
    CHECK(world.rooms.size() == data.rooms.size());
    const RoomState& hall = world.room("gate_hall");
    REQUIRE(hall.enemies.size() == 1);
    CHECK(hall.enemies[0].hp == data.enemy("rat").hp);
    CHECK(world.room("gatehouse").objects.at("desk").contents ==
          std::vector<ItemId>{"gate_key"});
    CHECK(world.room("gate_road").lit);
}
