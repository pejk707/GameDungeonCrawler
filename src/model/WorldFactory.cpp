#include "model/WorldFactory.h"

namespace ll {

WorldState WorldFactory::create(const GameData& data) {
    WorldState world;
    const PlayerStart& start = data.config.player;

    PlayerState& p = world.player;
    p.location = start.room;
    p.previousLocation = start.room;
    p.hp = p.maxHp = start.hp;
    p.baseAtk = start.atk;
    p.oil = start.oil;
    p.maxOil = start.oilMax;
    p.lanternLit = p.oil > 0;
    for (const ItemId& id : start.inventory) p.inventory.add(id);
    for (const ItemId& id : start.equipped) {
        const ItemDef* item = data.findItem(id);
        if (item && item->slot) p.equipment.slot(*item->slot) = id;
    }

    for (const RoomId& id : data.roomOrder) {
        const RoomDef& def = data.room(id);
        RoomState rs;
        rs.lit = def.litByDefault;
        rs.items = def.items;
        for (const EnemyPlacement& placement : def.enemies) {
            EnemyInstance e;
            e.def = placement.enemy;
            e.hp = data.enemy(placement.enemy).hp;
            e.behavior = placement.behavior;
            rs.enemies.push_back(e);
        }
        for (const ObjectDef& obj : def.objects) {
            ObjectState os;
            os.contents = obj.contents;
            os.usesLeft = obj.uses;
            rs.objects[obj.id] = os;
        }
        world.rooms[id] = std::move(rs);
    }
    world.room(p.location).visited = true;
    return world;
}

}  // namespace ll
