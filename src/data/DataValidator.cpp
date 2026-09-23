#include "data/DataValidator.h"

#include <map>
#include <set>
#include <utility>

namespace ll {
namespace {

class Checker {
public:
    explicit Checker(const GameData& data) : data_(data) {}

    std::vector<std::string> run() {
        checkConfig();
        checkZones();
        checkRooms();
        checkItems();
        checkEnemies();
        return std::move(errors_);
    }

private:
    void error(const std::string& where, const std::string& what) { errors_.push_back(where + ": " + what); }

    void item(const std::string& where, const ItemId& id) {
        if (!data_.findItem(id)) error(where, "неизвестный предмет '" + id + "'");
    }
    void room(const std::string& where, const RoomId& id) {
        if (!data_.findRoom(id)) error(where, "неизвестная комната '" + id + "'");
    }
    void enemy(const std::string& where, const EnemyId& id) {
        if (!data_.findEnemy(id)) error(where, "неизвестный враг '" + id + "'");
    }
    void text(const std::string& where, const std::string& path) {
        if (!path.empty() && !data_.texts.has(path)) error(where, "нет файла '" + path + "'");
    }
    void exit(const std::string& where, const RoomId& roomId, Direction dir) {
        const RoomDef* r = data_.findRoom(roomId);
        if (!r) {
            error(where, "неизвестная комната '" + roomId + "'");
        } else if (!r->exits.count(dir)) {
            error(where, "в комнате '" + roomId + "' нет выхода '" + std::string(toKey(dir)) + "'");
        }
    }

    void conditions(const std::string& where, const std::vector<Condition>& list) {
        for (const auto& c : list) {
            if (const auto* h = std::get_if<HasItem>(&c)) item(where, h->item);
        }
    }

    void effects(const std::string& where, const std::vector<Effect>& list) {
        for (const auto& e : list) {
            std::visit(overloaded{
                           [&](const GiveItem& g) { item(where, g.item); },
                           [&](const ConsumeItem& c) { item(where, c.item); },
                           [&](const RevealExit& r) { exit(where, r.room, r.dir); },
                           [&](const UnlockExit& u) { exit(where, u.room, u.dir); },
                           [&](const SpawnEnemy& s) { enemy(where, s.enemy); },
                           [&](const auto&) {},
                       },
                       e);
        }
    }

    void checkConfig() {
        const GameConfig& c = data_.config;
        room("config.json: start_room", c.player.room);
        for (const auto& id : c.player.inventory) item("config.json: inventory", id);
        for (const auto& id : c.player.equipped) {
            item("config.json: equipped", id);
            if (const ItemDef* it = data_.findItem(id); it && !it->slot) {
                error("config.json: equipped", "предмет '" + id + "' нельзя надеть");
            }
        }
        for (const auto& id : c.embers) item("config.json: embers", id);
        const GameTexts& t = c.texts;
        for (const std::string* p : {&t.title, &t.victory, &t.death, &t.intro, &t.epilogueA, &t.epilogueB}) {
            text("config.json: texts", *p);
        }
    }

    void checkZones() {
        for (const auto& [id, zone] : data_.zones) text("rooms.json: зона '" + id + "'", zone.banner);
    }

    void checkRooms() {
        std::map<std::pair<int, int>, RoomId> coords;
        for (const RoomId& id : data_.roomOrder) {
            const RoomDef& r = data_.room(id);
            const std::string where = "rooms.json: комната '" + id + "'";
            if (!data_.findZone(r.zone)) error(where, "неизвестная зона '" + r.zone + "'");
            const auto pos = std::make_pair(r.map.x, r.map.y);
            if (auto it = coords.find(pos); it != coords.end()) {
                error(where, "координаты карты совпадают с комнатой '" + it->second + "'");
            } else {
                coords[pos] = id;
            }
            if (r.description.empty()) error(where, "нет описания");
            for (const auto& [dir, e] : r.exits) {
                const std::string w = where + ", выход '" + std::string(toKey(dir)) + "'";
                room(w, e.to);
                if (e.lock) conditions(w, e.lock->conditions);
            }
            for (const auto& itemId : r.items) item(where, itemId);
            for (const auto& p : r.enemies) enemy(where, p.enemy);
            std::set<ObjectId> objectIds;
            for (const auto& o : r.objects) {
                const std::string w = where + ", объект '" + o.id + "'";
                if (!objectIds.insert(o.id).second) error(w, "повторяющийся id объекта");
                if (o.stems.empty()) error(w, "нет основ слов (stems)");
                if (o.kind == ObjectKind::Readable && o.text.empty()) error(w, "нет текста для чтения");
                for (const auto& c : o.contents) item(w, c);
                for (const auto& i : o.interactions) {
                    if (i.useItem) item(w, *i.useItem);
                    conditions(w, i.conditions);
                    effects(w, i.effects);
                    effects(w, i.onFail);
                }
            }
        }
    }

    void checkItems() {
        for (const auto& [id, it] : data_.items) {
            const std::string where = "items.json: предмет '" + id + "'";
            if (it.stems.empty()) error(where, "нет основ слов (stems)");
            if ((it.type == ItemType::Weapon || it.type == ItemType::Armor) && !it.slot) {
                error(where, "у оружия и брони должен быть слот");
            }
            if (it.type == ItemType::Note) {
                if (it.textFile.empty()) error(where, "у записки нет текста");
                text(where, it.textFile);
            }
        }
    }

    void checkEnemies() {
        for (const auto& [id, e] : data_.enemies) {
            const std::string where = "enemies.json: враг '" + id + "'";
            if (e.pattern.empty()) error(where, "пустой паттерн намерений");
            if (e.stems.empty()) error(where, "нет основ слов (stems)");
            std::set<IntentType> used(e.pattern.begin(), e.pattern.end());
            for (const auto& ph : e.phases) {
                if (ph.pattern.empty()) error(where, "пустой паттерн фазы");
                used.insert(ph.pattern.begin(), ph.pattern.end());
            }
            for (IntentType t : used) {
                if (!e.intentText.count(t)) {
                    error(where, "нет текста намерения '" + std::string(toKey(t)) + "'");
                }
            }
            for (const auto& l : e.loot) item(where, l);
            text(where, e.art);
        }
    }

    const GameData& data_;
    std::vector<std::string> errors_;
};

}  // namespace

std::vector<std::string> DataValidator::validate(const GameData& data) const {
    return Checker(data).run();
}

}  // namespace ll
