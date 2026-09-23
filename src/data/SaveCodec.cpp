#include "data/SaveCodec.h"

#include "data/DataLoader.h"
#include "model/WorldFactory.h"

namespace ll {

using json = nlohmann::json;

namespace {

json optionalId(const std::optional<ItemId>& id) { return id ? json(*id) : json(nullptr); }

std::optional<ItemId> readOptional(const json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return std::nullopt;
    return it->get<std::string>();
}

json directions(const std::set<Direction>& dirs) {
    json arr = json::array();
    for (Direction d : dirs) arr.push_back(std::string(toKey(d)));
    return arr;
}

std::set<Direction> readDirections(const json& j) {
    std::set<Direction> out;
    for (const auto& s : j) {
        if (auto d = fromKey<Direction>(s.get<std::string>())) out.insert(*d);
    }
    return out;
}

void requireItem(const GameData& data, const ItemId& id) {
    if (!data.findItem(id)) throw DataError("неизвестный предмет '" + id + "'");
}

}  // namespace

json SaveCodec::toJson(const WorldState& w) {
    const PlayerState& p = w.player;
    json inventory = json::array();
    for (const auto& [id, n] : p.inventory.entries()) inventory.push_back({id, n});

    json player = {{"location", p.location},
                   {"previous", p.previousLocation},
                   {"hp", p.hp},
                   {"max_hp", p.maxHp},
                   {"base_atk", p.baseAtk},
                   {"oil", p.oil},
                   {"max_oil", p.maxOil},
                   {"lantern", p.lanternLit},
                   {"darkness", p.darknessCounter},
                   {"inventory", inventory},
                   {"equipment",
                    {{"weapon", optionalId(p.equipment.weapon)},
                     {"body", optionalId(p.equipment.body)},
                     {"head", optionalId(p.equipment.head)}}},
                   {"upgrades", p.upgrades},
                   {"notes", p.readNotes}};

    json rooms = json::object();
    for (const auto& [id, r] : w.rooms) {
        json enemies = json::array();
        for (const auto& e : r.enemies) {
            enemies.push_back({{"def", e.def},
                               {"hp", e.hp},
                               {"behavior", std::string(toKey(e.behavior))},
                               {"wake_in", e.wakeIn},
                               {"pattern", e.patternIndex},
                               {"phase", e.phase},
                               {"exposed", e.exposed},
                               {"engaged", e.engaged}});
        }
        json objects = json::object();
        for (const auto& [objId, o] : r.objects) {
            objects[objId] = {{"opened", o.opened},
                              {"contents", o.contents},
                              {"uses", o.usesLeft},
                              {"done", o.doneInteractions}};
        }
        rooms[id] = {{"visited", r.visited},
                     {"lit", r.lit},
                     {"items", r.items},
                     {"enemies", enemies},
                     {"revealed", directions(r.revealedExits)},
                     {"unlocked", directions(r.unlockedExits)},
                     {"objects", objects}};
    }

    return {{"version", kVersion},
            {"player", player},
            {"flags", w.flags},
            {"rooms", rooms},
            {"stats",
             {{"turns", w.stats.turns},
              {"oil_burned", w.stats.oilBurned},
              {"deaths", w.stats.deaths},
              {"kills", w.stats.kills}}},
            {"hints", w.shownHints}};
}

WorldState SaveCodec::fromJson(const json& j, const GameData& data) {
    try {
        if (j.at("version").get<int>() != kVersion) throw DataError("неподдерживаемая версия сохранения");
        // Начинаем с нового мира: комнаты, которых нет в сохранении, останутся в исходном виде.
        WorldState w = WorldFactory::create(data);

        const json& p = j.at("player");
        PlayerState& ps = w.player;
        ps.location = p.at("location").get<std::string>();
        ps.previousLocation = p.at("previous").get<std::string>();
        if (!data.findRoom(ps.location)) throw DataError("неизвестная комната '" + ps.location + "'");
        if (!data.findRoom(ps.previousLocation)) ps.previousLocation = ps.location;
        ps.hp = p.at("hp").get<int>();
        ps.maxHp = p.at("max_hp").get<int>();
        ps.baseAtk = p.at("base_atk").get<int>();
        ps.oil = p.at("oil").get<int>();
        ps.maxOil = p.at("max_oil").get<int>();
        ps.lanternLit = p.at("lantern").get<bool>();
        ps.darknessCounter = p.at("darkness").get<int>();
        ps.inventory = Inventory{};
        for (const auto& entry : p.at("inventory")) {
            const ItemId id = entry.at(0).get<std::string>();
            requireItem(data, id);
            ps.inventory.add(id, entry.at(1).get<int>());
        }
        const json& eq = p.at("equipment");
        ps.equipment.weapon = readOptional(eq, "weapon");
        ps.equipment.body = readOptional(eq, "body");
        ps.equipment.head = readOptional(eq, "head");
        for (const auto* slot : {&ps.equipment.weapon, &ps.equipment.body, &ps.equipment.head}) {
            if (*slot) requireItem(data, **slot);
        }
        ps.upgrades = p.at("upgrades").get<std::set<ItemId>>();
        ps.readNotes = p.at("notes").get<std::set<ItemId>>();
        for (const auto& id : ps.upgrades) requireItem(data, id);
        for (const auto& id : ps.readNotes) requireItem(data, id);

        w.flags = j.at("flags").get<std::set<FlagId>>();
        for (const auto& [id, r] : j.at("rooms").items()) {
            const RoomDef* def = data.findRoom(id);
            if (!def) continue;  // комнату убрали из данных — пропускаем
            RoomState rs;
            rs.visited = r.at("visited").get<bool>();
            rs.lit = r.at("lit").get<bool>();
            rs.items = r.at("items").get<std::vector<ItemId>>();
            for (const auto& item : rs.items) requireItem(data, item);
            for (const auto& e : r.at("enemies")) {
                EnemyInstance inst;
                inst.def = e.at("def").get<std::string>();
                if (!data.findEnemy(inst.def)) throw DataError("неизвестный враг '" + inst.def + "'");
                inst.hp = e.at("hp").get<int>();
                inst.behavior = fromKey<Behavior>(e.at("behavior").get<std::string>()).value_or(Behavior::Aggressive);
                inst.wakeIn = e.at("wake_in").get<int>();
                inst.patternIndex = e.at("pattern").get<int>();
                inst.phase = e.at("phase").get<int>();
                inst.exposed = e.at("exposed").get<bool>();
                inst.engaged = e.value("engaged", false);
                rs.enemies.push_back(inst);
            }
            rs.revealedExits = readDirections(r.at("revealed"));
            rs.unlockedExits = readDirections(r.at("unlocked"));
            rs.objects = w.room(id).objects;  // объекты, которых нет в сохранении, — как в новом мире
            for (const auto& [objId, o] : r.at("objects").items()) {
                if (!def->findObject(objId)) continue;
                ObjectState os;
                os.opened = o.at("opened").get<bool>();
                os.contents = o.at("contents").get<std::vector<ItemId>>();
                for (const auto& item : os.contents) requireItem(data, item);
                os.usesLeft = o.at("uses").get<int>();
                os.doneInteractions = o.at("done").get<std::set<int>>();
                rs.objects[objId] = os;
            }
            w.rooms[id] = std::move(rs);
        }
        const json& s = j.at("stats");
        w.stats.turns = s.at("turns").get<int>();
        w.stats.oilBurned = s.at("oil_burned").get<int>();
        w.stats.deaths = s.at("deaths").get<int>();
        w.stats.kills = s.at("kills").get<int>();
        w.shownHints = j.at("hints").get<std::set<HintId>>();
        return w;
    } catch (const json::exception& e) {
        throw DataError(std::string("повреждённое сохранение: ") + e.what());
    }
}

}  // namespace ll
