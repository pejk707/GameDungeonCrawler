#include "data/DataLoader.h"

#include <fstream>
#include <set>
#include <sstream>

#include "common/Utf8.h"
#include "nlohmann/json.hpp"

namespace ll {

using json = nlohmann::json;

namespace {

// ---------- помощники разбора ----------

template <class T>
T opt(const json& j, const char* key, T fallback) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return fallback;
    return it->get<T>();
}

std::string str(const json& j, const char* key) { return opt<std::string>(j, key, {}); }

std::vector<std::string> strList(const json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return {};
    return it->get<std::vector<std::string>>();
}

std::vector<std::string> foldedList(const json& j, const char* key) {
    std::vector<std::string> out;
    for (const auto& s : strList(j, key)) out.push_back(utf8::fold(s));
    return out;
}

template <class E>
E parseEnum(const std::string& value, const char* what) {
    if (auto e = fromKey<E>(value)) return *e;
    throw DataError(std::string("неизвестное значение ") + what + " '" + value + "'");
}

template <class E>
E enumField(const json& j, const char* key, E fallback, const char* what) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return fallback;
    return parseEnum<E>(it->get<std::string>(), what);
}

MapPos parseMap(const json& j) {
    auto it = j.find("map");
    if (it == j.end()) return {};
    return {it->at(0).get<int>(), it->at(1).get<int>()};
}

// Условие — объект с одним ключом: {"has_flag": "x"}.
Condition parseCondition(const json& j) {
    if (!j.is_object() || j.size() != 1) throw DataError("условие должно быть объектом с одним ключом");
    const auto& [key, v] = *j.items().begin();
    if (key == "has_flag") return HasFlag{v.get<std::string>()};
    if (key == "not_flag") return NotFlag{v.get<std::string>()};
    if (key == "has_item") return HasItem{v.get<std::string>()};
    if (key == "embers") return EmberCount{v.get<int>()};
    throw DataError("неизвестное условие '" + key + "'");
}

std::vector<Condition> parseConditions(const json& j, const char* key) {
    std::vector<Condition> out;
    auto it = j.find(key);
    if (it == j.end()) return out;
    for (const auto& c : *it) out.push_back(parseCondition(c));
    return out;
}

RevealExit parseExitRef(const json& v, const RoomId& currentRoom) {
    RevealExit r;
    if (v.is_string()) {
        r.room = currentRoom;
        r.dir = parseEnum<Direction>(v.get<std::string>(), "направления");
    } else {
        r.room = opt<std::string>(v, "room", currentRoom);
        r.dir = parseEnum<Direction>(v.at("dir").get<std::string>(), "направления");
    }
    return r;
}

// Эффект — объект с одним ключом: {"set_flag": "x"}, {"spend_oil": 5}, ...
Effect parseEffect(const json& j, const RoomId& room) {
    if (!j.is_object() || j.size() != 1) throw DataError("эффект должен быть объектом с одним ключом");
    const auto& [key, v] = *j.items().begin();
    if (key == "set_flag") return SetFlag{v.get<std::string>()};
    if (key == "clear_flags") return ClearFlags{v.get<std::vector<std::string>>()};
    if (key == "give_item") {
        if (v.is_string()) return GiveItem{v.get<std::string>(), 1};
        return GiveItem{v.at("item").get<std::string>(), opt<int>(v, "count", 1)};
    }
    if (key == "consume_item") {
        if (v.is_string()) return ConsumeItem{v.get<std::string>(), 1};
        return ConsumeItem{v.at("item").get<std::string>(), opt<int>(v, "count", 1)};
    }
    if (key == "reveal_exit") return parseExitRef(v, room);
    if (key == "unlock_exit") {
        const RevealExit r = parseExitRef(v, room);
        return UnlockExit{r.room, r.dir};
    }
    if (key == "spawn_enemy") return SpawnEnemy{v.get<std::string>()};
    if (key == "spend_oil") return SpendOil{v.get<int>()};
    if (key == "add_oil") return AddOil{v.get<int>()};
    if (key == "light_room") return LightRoom{};
    if (key == "text") return ShowText{v.get<std::string>()};
    if (key == "win_game") return WinGame{};
    throw DataError("неизвестный эффект '" + key + "'");
}

std::vector<Effect> parseEffects(const json& j, const char* key, const RoomId& room) {
    std::vector<Effect> out;
    auto it = j.find(key);
    if (it == j.end()) return out;
    for (const auto& e : *it) out.push_back(parseEffect(e, room));
    return out;
}

InteractionDef parseInteraction(const json& j, const RoomId& room) {
    InteractionDef d;
    d.trigger = parseEnum<Trigger>(j.at("trigger").get<std::string>(), "триггера");
    if (j.contains("item")) d.useItem = j.at("item").get<std::string>();
    d.conditions = parseConditions(j, "conditions");
    d.effects = parseEffects(j, "effects", room);
    d.onFail = parseEffects(j, "on_fail", room);
    d.message = str(j, "message");
    d.failMessage = str(j, "fail_message");
    d.once = opt<bool>(j, "once", false);
    return d;
}

ObjectDef parseObject(const json& j, const RoomId& room) {
    ObjectDef o;
    o.id = j.at("id").get<std::string>();
    o.name = j.at("name").get<std::string>();
    o.stems = foldedList(j, "stems");
    o.kind = enumField<ObjectKind>(j, "kind", ObjectKind::Plain, "вида объекта");
    o.description = str(j, "description");
    o.text = str(j, "text");
    o.contents = strList(j, "contents");
    o.uses = opt<int>(j, "uses", -1);
    if (auto it = j.find("interactions"); it != j.end()) {
        for (const auto& i : *it) o.interactions.push_back(parseInteraction(i, room));
    }
    return o;
}

ExitDef parseExit(Direction dir, const json& v, const RoomId& room) {
    ExitDef e;
    e.dir = dir;
    if (v.is_string()) {
        e.to = v.get<std::string>();
        return e;
    }
    e.to = v.at("to").get<std::string>();
    e.hidden = opt<bool>(v, "hidden", false);
    if (auto it = v.find("lock"); it != v.end()) {
        LockDef lock;
        lock.conditions = parseConditions(*it, "conditions");
        lock.lockedMessage = str(*it, "locked");
        lock.unlockMessage = str(*it, "unlocked");
        e.lock = lock;
    }
    (void)room;
    return e;
}

RoomDef parseRoom(const json& j) {
    RoomDef r;
    r.id = j.at("id").get<std::string>();
    r.name = j.at("name").get<std::string>();
    r.mapLabel = opt<std::string>(j, "map_label", r.name);
    r.zone = j.at("zone").get<std::string>();
    r.map = parseMap(j);
    r.litByDefault = opt<bool>(j, "lit", false);
    r.description = str(j, "description");
    r.descriptionDark = str(j, "description_dark");
    if (auto it = j.find("exits"); it != j.end()) {
        for (const auto& [key, v] : it->items()) {
            const Direction dir = parseEnum<Direction>(key, "направления");
            r.exits[dir] = parseExit(dir, v, r.id);
        }
    }
    r.items = strList(j, "items");
    if (auto it = j.find("objects"); it != j.end()) {
        for (const auto& o : *it) r.objects.push_back(parseObject(o, r.id));
    }
    if (auto it = j.find("enemies"); it != j.end()) {
        for (const auto& e : *it) {
            EnemyPlacement p;
            if (e.is_string()) {
                p.enemy = e.get<std::string>();
            } else {
                p.enemy = e.at("enemy").get<std::string>();
                p.behavior = enumField<Behavior>(e, "behavior", Behavior::Aggressive, "поведения");
            }
            r.enemies.push_back(p);
        }
    }
    return r;
}

ItemDef parseItem(const json& j) {
    ItemDef it;
    it.id = j.at("id").get<std::string>();
    it.name = j.at("name").get<std::string>();
    it.stems = foldedList(j, "stems");
    it.type = parseEnum<ItemType>(j.at("type").get<std::string>(), "типа предмета");
    if (j.contains("slot")) it.slot = parseEnum<EquipSlot>(j.at("slot").get<std::string>(), "слота");
    it.atkBonus = opt<int>(j, "atk", 0);
    it.defBonus = opt<int>(j, "def", 0);
    it.flareBonus = opt<int>(j, "flare", 0);
    if (auto e = j.find("effect"); e != j.end()) {
        it.effect.heal = opt<int>(*e, "heal", 0);
        it.effect.oil = opt<int>(*e, "oil", 0);
        it.effect.damage = opt<int>(*e, "damage", 0);
        it.effect.maxHp = opt<int>(*e, "max_hp", 0);
        it.effect.maxOil = opt<int>(*e, "max_oil", 0);
        it.effect.escape = opt<bool>(*e, "escape", false);
    }
    const bool defaultDroppable = it.type != ItemType::Key && it.type != ItemType::Lantern;
    it.droppable = opt<bool>(j, "droppable", defaultDroppable);
    it.combatOnly = opt<bool>(j, "combat_only", false);
    it.description = str(j, "description");
    it.textFile = str(j, "text");
    it.useMessage = str(j, "use_message");
    return it;
}

std::vector<IntentType> parsePattern(const json& j, const char* key) {
    std::vector<IntentType> out;
    for (const auto& s : strList(j, key)) out.push_back(parseEnum<IntentType>(s, "намерения"));
    return out;
}

EnemyDef parseEnemy(const json& j) {
    EnemyDef e;
    e.id = j.at("id").get<std::string>();
    e.name = j.at("name").get<std::string>();
    e.stems = foldedList(j, "stems");
    e.description = str(j, "description");
    e.hp = j.at("hp").get<int>();
    e.atk = j.at("atk").get<int>();
    e.def = opt<int>(j, "def", 0);
    for (const auto& t : strList(j, "traits")) e.traits.insert(parseEnum<Trait>(t, "черты"));
    e.dodgeChance = opt<double>(j, "dodge", 0.0);
    e.snuffAmount = opt<int>(j, "snuff", 10);
    e.drainAmount = opt<int>(j, "drain", 20);
    e.drainBlocked = opt<int>(j, "drain_blocked", 5);
    e.pattern = parsePattern(j, "pattern");
    if (auto it = j.find("phases"); it != j.end()) {
        for (const auto& p : *it) {
            PhaseDef ph;
            ph.hpBelow = p.at("hp_below").get<double>();
            ph.pattern = parsePattern(p, "pattern");
            ph.message = str(p, "message");
            e.phases.push_back(ph);
        }
    }
    if (auto it = j.find("intents"); it != j.end()) {
        for (const auto& [key, v] : it->items()) {
            e.intentText[parseEnum<IntentType>(key, "намерения")] = v.get<std::string>();
        }
    }
    e.loot = strList(j, "loot");
    e.introText = str(j, "intro");
    e.defeatText = str(j, "defeat");
    e.sleepText = str(j, "sleep");
    e.art = str(j, "art");
    return e;
}

void flattenStrings(const json& j, const std::string& prefix, StringTable& out) {
    for (const auto& [key, v] : j.items()) {
        const std::string full = prefix.empty() ? key : prefix + "." + key;
        if (v.is_object()) {
            flattenStrings(v, full, out);
        } else if (v.is_array()) {
            std::string joined;
            for (const auto& line : v) joined += line.get<std::string>() + "\n";
            if (!joined.empty()) joined.pop_back();
            out.set(full, joined);
        } else {
            out.set(full, v.get<std::string>());
        }
    }
}

std::vector<std::string> splitWords(const std::string& phrase) {
    std::vector<std::string> words;
    std::istringstream in(utf8::fold(phrase));
    std::string w;
    while (in >> w) words.push_back(w);
    return words;
}

}  // namespace

// ---------- DataLoader ----------

std::string DataLoader::readTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw DataError("не удалось открыть файл " + path.generic_string());
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string s = ss.str();
    if (s.size() >= 3 && s.compare(0, 3, "\xEF\xBB\xBF") == 0) s.erase(0, 3);
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c != '\r') out += c;
    }
    return out;
}

namespace {

json readJson(const std::filesystem::path& root, const std::string& rel) {
    const std::string text = DataLoader::readTextFile(root / rel);
    try {
        return json::parse(text);
    } catch (const json::parse_error& e) {
        throw DataError(rel + ": неверный JSON: " + e.what());
    }
}

}  // namespace

GameData DataLoader::loadAll() const {
    GameData data;
    loadConfig(data);
    loadRooms(data);
    loadItems(data);
    loadEnemies(data);
    loadStrings(data);
    loadLexicon(data);
    loadTexts(data);
    return data;
}

void DataLoader::loadConfig(GameData& data) const {
    const std::string file = "data/config.json";
    const json j = readJson(root_, file);
    try {
        GameConfig& c = data.config;
        const json& p = j.at("player");
        c.player.room = p.at("start_room").get<std::string>();
        c.player.hp = opt<int>(p, "hp", 30);
        c.player.atk = opt<int>(p, "atk", 1);
        c.player.oil = opt<int>(p, "oil", 80);
        c.player.oilMax = opt<int>(p, "oil_max", 100);
        c.player.inventory = strList(p, "inventory");
        c.player.equipped = strList(p, "equipped");
        if (auto it = j.find("oil_cost"); it != j.end()) {
            c.oilCost.moveDark = opt<int>(*it, "move_dark", 2);
            c.oilCost.flare = opt<int>(*it, "flare", 8);
            c.oilCost.brazier = opt<int>(*it, "brazier", 10);
        }
        if (auto it = j.find("combat"); it != j.end()) {
            CombatConfig& cc = c.combat;
            cc.critChance = opt<double>(*it, "crit_chance", cc.critChance);
            cc.critMult = opt<double>(*it, "crit_mult", cc.critMult);
            cc.blockMult = opt<double>(*it, "block_mult", cc.blockMult);
            cc.counterMult = opt<double>(*it, "counter_mult", cc.counterMult);
            cc.guardMult = opt<double>(*it, "guard_mult", cc.guardMult);
            cc.fleeChance = opt<double>(*it, "flee_chance", cc.fleeChance);
            cc.darkMissChance = opt<double>(*it, "dark_miss_chance", cc.darkMissChance);
            cc.photophobiaMult = opt<double>(*it, "photophobia_mult", cc.photophobiaMult);
            cc.flareDamage = opt<int>(*it, "flare_damage", cc.flareDamage);
        }
        if (auto it = j.find("score"); it != j.end()) {
            ScoreConfig& s = c.score;
            s.base = opt<int>(*it, "base", s.base);
            s.perOil = opt<int>(*it, "per_oil", s.perOil);
            s.perNote = opt<int>(*it, "per_note", s.perNote);
            s.perTurn = opt<int>(*it, "per_turn", s.perTurn);
            s.perDeath = opt<int>(*it, "per_death", s.perDeath);
            s.master = opt<int>(*it, "master", s.master);
            s.lamplighter = opt<int>(*it, "lamplighter", s.lamplighter);
        }
        if (auto it = j.find("texts"); it != j.end()) {
            c.texts.title = str(*it, "title");
            c.texts.victory = str(*it, "victory");
            c.texts.death = str(*it, "death");
            c.texts.intro = str(*it, "intro");
            c.texts.epilogueA = str(*it, "epilogue_a");
            c.texts.epilogueB = str(*it, "epilogue_b");
        }
        c.darknessTurnsToDeath = opt<int>(j, "darkness_turns_to_death", 4);
        if (j.contains("oil_warnings")) c.oilWarnings = j.at("oil_warnings").get<std::vector<int>>();
        c.embers = strList(j, "embers");
        c.totalNotes = opt<int>(j, "total_notes", 8);
        c.width = opt<int>(j, "width", 78);
    } catch (const json::exception& e) {
        throw DataError(file + ": " + e.what());
    }
}

void DataLoader::loadRooms(GameData& data) const {
    const std::string file = "data/world/rooms.json";
    const json j = readJson(root_, file);
    try {
        for (const auto& z : j.at("zones")) {
            ZoneDef zone{z.at("id").get<std::string>(), z.at("name").get<std::string>(), str(z, "banner")};
            data.zones[zone.id] = zone;
        }
    } catch (const std::exception& e) {
        throw DataError(file + ": zones: " + e.what());
    }
    for (const auto& r : j.at("rooms")) {
        const std::string id = opt<std::string>(r, "id", "?");
        try {
            RoomDef room = parseRoom(r);
            if (data.rooms.count(room.id)) throw DataError("повторяющийся id");
            data.roomOrder.push_back(room.id);
            data.rooms[room.id] = std::move(room);
        } catch (const std::exception& e) {
            throw DataError(file + ": комната '" + id + "': " + e.what());
        }
    }
}

void DataLoader::loadItems(GameData& data) const {
    const std::string file = "data/world/items.json";
    for (const auto& i : readJson(root_, file)) {
        const std::string id = opt<std::string>(i, "id", "?");
        try {
            ItemDef item = parseItem(i);
            if (data.items.count(item.id)) throw DataError("повторяющийся id");
            data.items[item.id] = std::move(item);
        } catch (const std::exception& e) {
            throw DataError(file + ": предмет '" + id + "': " + e.what());
        }
    }
}

void DataLoader::loadEnemies(GameData& data) const {
    const std::string file = "data/world/enemies.json";
    for (const auto& en : readJson(root_, file)) {
        const std::string id = opt<std::string>(en, "id", "?");
        try {
            EnemyDef enemy = parseEnemy(en);
            if (data.enemies.count(enemy.id)) throw DataError("повторяющийся id");
            data.enemies[enemy.id] = std::move(enemy);
        } catch (const std::exception& e) {
            throw DataError(file + ": враг '" + id + "': " + e.what());
        }
    }
}

void DataLoader::loadStrings(GameData& data) const {
    const std::string file = "data/lang/ru/strings.json";
    try {
        flattenStrings(readJson(root_, file), "", data.strings);
    } catch (const json::exception& e) {
        throw DataError(file + ": " + e.what());
    }
}

void DataLoader::loadLexicon(GameData& data) const {
    const std::string file = "data/lang/ru/commands.json";
    const json j = readJson(root_, file);
    try {
        const std::pair<const char*, ParseMode> modes[] = {{"exploration", ParseMode::Exploration},
                                                          {"combat", ParseMode::Combat}};
        for (const auto& [section, mode] : modes) {
            for (const auto& [key, phrases] : j.at(section).items()) {
                const auto verb = verbFromKey(key);
                if (!verb) throw DataError(std::string(section) + ": неизвестная команда '" + key + "'");
                for (const auto& phrase : phrases) {
                    data.lexicon.addVerb(mode, splitWords(phrase.get<std::string>()), *verb);
                }
            }
        }
        for (const auto& [key, words] : j.at("directions").items()) {
            const Direction dir = parseEnum<Direction>(key, "направления");
            for (const auto& w : words) data.lexicon.addDirection(utf8::fold(w.get<std::string>()), dir);
        }
        for (const auto& w : j.at("prepositions")) data.lexicon.addPreposition(utf8::fold(w.get<std::string>()));
        for (const auto& w : j.at("noise")) data.lexicon.addNoise(utf8::fold(w.get<std::string>()));
        for (const auto& w : j.at("all")) data.lexicon.addAll(utf8::fold(w.get<std::string>()));
    } catch (const json::exception& e) {
        throw DataError(file + ": " + e.what());
    }
}

void DataLoader::loadTexts(GameData& data) const {
    // Собираем все пути к текстовым файлам, на которые ссылаются данные.
    std::set<std::string> paths;
    const GameTexts& t = data.config.texts;
    for (const std::string* p : {&t.title, &t.victory, &t.death, &t.intro, &t.epilogueA, &t.epilogueB}) {
        if (!p->empty()) paths.insert(*p);
    }
    for (const auto& [id, zone] : data.zones) {
        if (!zone.banner.empty()) paths.insert(zone.banner);
    }
    for (const auto& [id, item] : data.items) {
        if (!item.textFile.empty()) paths.insert(item.textFile);
    }
    for (const auto& [id, enemy] : data.enemies) {
        if (!enemy.art.empty()) paths.insert(enemy.art);
    }
    // Отсутствующие файлы не ломают загрузку — о них сообщит DataValidator.
    for (const std::string& rel : paths) {
        const std::filesystem::path full = root_ / rel;
        if (std::filesystem::exists(full)) data.texts.set(rel, readTextFile(full));
    }
}

}  // namespace ll
