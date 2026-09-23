#include "parser/EntityResolver.h"

#include <algorithm>

#include "common/Utf8.h"
#include "core/GameContext.h"

namespace ll {

int EntityResolver::score(const std::vector<std::string>& words, const std::vector<std::string>& stems) {
    int s = 0;
    for (const auto& w : words) {
        for (const auto& stem : stems) {
            if (!stem.empty() && utf8::startsWith(w, stem)) {
                ++s;
                break;
            }
        }
    }
    return s;
}

std::string EntityResolver::name(const EntityRef& ref, const GameContext& ctx) {
    switch (ref.kind) {
        case EntityKind::Inventory:
        case EntityKind::Floor:
        case EntityKind::Container:
            return ctx.data.item(ref.id).name;
        case EntityKind::Object:
            if (const ObjectDef* o = ctx.data.room(ctx.world.player.location).findObject(ref.id)) return o->name;
            return ref.id;
        case EntityKind::Enemy:
            return ctx.data.enemy(ref.id).name;
    }
    return ref.id;
}

Resolution EntityResolver::resolve(const std::vector<std::string>& words, unsigned scopeMask,
                                   const GameContext& ctx, bool canSee) const {
    struct Candidate {
        EntityRef ref;
        int score;
    };
    std::vector<Candidate> candidates;
    auto consider = [&](EntityRef ref, const std::vector<std::string>& stems) {
        const int s = score(words, stems);
        if (s > 0) candidates.push_back({std::move(ref), s});
    };

    const RoomDef& roomDef = ctx.data.room(ctx.world.player.location);
    const RoomState& room = ctx.world.currentRoom();

    if (scopeMask & scope::Inventory) {
        for (const auto& [id, count] : ctx.world.player.inventory.entries()) {
            consider({EntityKind::Inventory, id, 0, {}}, ctx.data.item(id).stems);
        }
    }
    if (canSee && (scopeMask & scope::Floor)) {
        for (const auto& id : room.items) consider({EntityKind::Floor, id, 0, {}}, ctx.data.item(id).stems);
    }
    if (canSee && (scopeMask & scope::Containers)) {
        for (const auto& [objId, os] : room.objects) {
            if (!os.opened) continue;
            for (const auto& id : os.contents) {
                consider({EntityKind::Container, id, 0, objId}, ctx.data.item(id).stems);
            }
        }
    }
    if (canSee && (scopeMask & scope::Objects)) {
        for (const auto& o : roomDef.objects) consider({EntityKind::Object, o.id, 0, {}}, o.stems);
    }
    // Врагов слышно и в темноте.
    if (scopeMask & scope::Enemies) {
        for (std::size_t i = 0; i < room.enemies.size(); ++i) {
            const auto& e = room.enemies[i];
            consider({EntityKind::Enemy, e.def, i, {}}, ctx.data.enemy(e.def).stems);
        }
    }

    Resolution res;
    if (candidates.empty()) return res;
    const int best = std::max_element(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
                         return a.score < b.score;
                     })->score;
    for (const auto& c : candidates) {
        if (c.score != best) continue;
        const bool dup = std::any_of(res.candidates.begin(), res.candidates.end(),
                                     [&](const EntityRef& r) { return r.id == c.ref.id; });
        if (!dup) res.candidates.push_back(c.ref);
    }
    if (res.candidates.size() == 1) {
        res.kind = Resolution::Kind::Found;
        res.ref = res.candidates.front();
    } else {
        res.kind = Resolution::Kind::Ambiguous;
    }
    return res;
}

}  // namespace ll
