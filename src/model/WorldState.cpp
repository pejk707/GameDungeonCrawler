#include "model/WorldState.h"

#include <algorithm>

namespace ll {

void Inventory::add(const ItemId& id, int count) {
    if (count <= 0) return;
    for (auto& [item, n] : entries_) {
        if (item == id) {
            n += count;
            return;
        }
    }
    entries_.emplace_back(id, count);
}

bool Inventory::remove(const ItemId& id, int count) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->first != id) continue;
        if (it->second < count) return false;
        it->second -= count;
        if (it->second == 0) entries_.erase(it);
        return true;
    }
    return false;
}

int Inventory::count(const ItemId& id) const {
    for (const auto& [item, n] : entries_) {
        if (item == id) return n;
    }
    return 0;
}

std::optional<ItemId>& Equipment::slot(EquipSlot s) {
    switch (s) {
        case EquipSlot::Weapon: return weapon;
        case EquipSlot::Body: return body;
        case EquipSlot::Head: return head;
    }
    return weapon;
}

const std::optional<ItemId>& Equipment::slot(EquipSlot s) const {
    return const_cast<Equipment*>(this)->slot(s);
}

bool Equipment::isEquipped(const ItemId& id) const {
    return weapon == id || body == id || head == id;
}

}  // namespace ll
