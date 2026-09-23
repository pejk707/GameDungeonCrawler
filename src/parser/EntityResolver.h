#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "common/Ids.h"

namespace ll {

struct GameContext;

enum class EntityKind { Inventory, Floor, Container, Object, Enemy };

struct EntityRef {
    EntityKind kind = EntityKind::Inventory;
    std::string id;         // ItemId, ObjectId или EnemyId
    std::size_t index = 0;  // для врагов — индекс в RoomState::enemies
    ObjectId container;     // для предметов в контейнере
};

namespace scope {
constexpr unsigned Inventory = 1;
constexpr unsigned Floor = 2;
constexpr unsigned Containers = 4;
constexpr unsigned Objects = 8;
constexpr unsigned Enemies = 16;
constexpr unsigned Visible = Floor | Containers | Objects | Enemies;
constexpr unsigned All = Inventory | Visible;
}  // namespace scope

struct Resolution {
    enum class Kind { Found, Ambiguous, NotFound };
    Kind kind = Kind::NotFound;
    EntityRef ref;
    std::vector<EntityRef> candidates;
    bool found() const { return kind == Kind::Found; }
};

// Находит предмет, объект или врага по словам игрока. Сравнение — по основам слов (stems):
// «рукоять», «рукоятью», «рукояти» подходят к основе «рукоят».
class EntityResolver {
public:
    Resolution resolve(const std::vector<std::string>& words, unsigned scopeMask, const GameContext& ctx,
                       bool canSee) const;
    static int score(const std::vector<std::string>& words, const std::vector<std::string>& stems);
    static std::string name(const EntityRef& ref, const GameContext& ctx);
};

}  // namespace ll
