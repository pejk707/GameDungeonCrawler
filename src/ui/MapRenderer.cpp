#include "ui/MapRenderer.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>

#include "common/Utf8.h"
#include "core/GameContext.h"
#include "core/Systems.h"

namespace ll {
namespace {

constexpr int kCell = 11;  // «[Название]» + отметка
constexpr int kGap = 3;    // соединитель между ячейками
constexpr int kPitch = kCell + kGap;
constexpr std::size_t kLabel = 8;

struct Canvas {
    std::vector<std::u32string> lines;
    void put(int row, int col, const std::string& text) {
        if (row < 0 || row >= static_cast<int>(lines.size())) return;
        std::u32string& line = lines[static_cast<std::size_t>(row)];
        const std::u32string u = utf8::decode(text);
        for (std::size_t i = 0; i < u.size(); ++i) {
            const std::size_t pos = static_cast<std::size_t>(col) + i;
            if (pos < line.size()) line[pos] = u[i];
        }
    }
};

bool adjacent(const MapPos& a, const MapPos& b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1; }

}  // namespace

std::string MapRenderer::render(const GameContext& ctx) {
    const RoomId& here = ctx.world.player.location;
    const ZoneId zone = ctx.data.room(here).zone;

    // Какие комнаты рисуем: посещённые в этой зоне и их известные соседи.
    std::set<RoomId> known;
    for (const auto& [id, rs] : ctx.world.rooms) {
        if ((rs.visited && ctx.data.room(id).zone == zone) || id == here) known.insert(id);
    }
    std::set<RoomId> shown = known;
    for (const RoomId& id : known) {
        for (Direction d : kAllDirections) {
            const ExitDef* exit = ctx.sys.movement.findExit(id, d, ctx);
            if (exit && adjacent(ctx.data.room(id).map, ctx.data.room(exit->to).map)) shown.insert(exit->to);
        }
    }

    int minX = 1 << 20, maxX = -(1 << 20), minY = 1 << 20, maxY = -(1 << 20);
    for (const RoomId& id : shown) {
        const MapPos& m = ctx.data.room(id).map;
        minX = std::min(minX, m.x);
        maxX = std::max(maxX, m.x);
        minY = std::min(minY, m.y);
        maxY = std::max(maxY, m.y);
    }
    const int cols = maxX - minX + 1;
    const int rows = (maxY - minY) * 2 + 1;
    Canvas canvas;
    canvas.lines.assign(static_cast<std::size_t>(rows), std::u32string(static_cast<std::size_t>(cols * kPitch), U' '));

    for (const RoomId& id : shown) {
        const RoomDef& def = ctx.data.room(id);
        const RoomState& rs = ctx.world.room(id);
        const int row = (def.map.y - minY) * 2;
        const int col = (def.map.x - minX) * kPitch;
        const bool isKnown = rs.visited || id == here;

        std::string label = isKnown ? utf8::truncate(def.mapLabel, kLabel) : std::string("   ?");
        // Переходы вверх/вниз в несоседние клетки показываем стрелкой.
        bool up = false, down = false;
        if (isKnown) {
            for (Direction d : {Direction::Up, Direction::Down}) {
                const ExitDef* exit = ctx.sys.movement.findExit(id, d, ctx);
                if (exit && !adjacent(def.map, ctx.data.room(exit->to).map)) (d == Direction::Up ? up : down) = true;
            }
        }
        std::string marker = " ";
        if (id == here) {
            marker = "@";
        } else if (isKnown && !rs.enemies.empty()) {
            marker = "!";
        } else if (isKnown && rs.lit && def.brazier()) {
            marker = "#";
        } else if (up || down) {
            marker = up && down ? "↕" : (up ? "↑" : "↓");
        }
        canvas.put(row, col, "[" + utf8::padRight(label, kLabel) + "]" + marker);
        if (!isKnown) continue;

        for (Direction d : kAllDirections) {
            const ExitDef* exit = ctx.sys.movement.findExit(id, d, ctx);
            if (!exit) continue;
            const MapPos& to = ctx.data.room(exit->to).map;
            const bool vertical = d == Direction::Up || d == Direction::Down;
            if (!adjacent(def.map, to)) continue;
            if (to.y == def.map.y) {  // сосед по горизонтали
                const int left = std::min(def.map.x, to.x) - minX;
                canvas.put(row, left * kPitch + kCell, vertical ? " · " : "───");
            } else {  // сосед по вертикали
                const int upper = std::min(def.map.y, to.y) - minY;
                canvas.put(upper * 2 + 1, col + kCell / 2 - 1, vertical ? ":" : "│");
            }
        }
    }

    std::string out;
    for (const auto& line : canvas.lines) {
        std::string s = utf8::encode(line);
        while (!s.empty() && s.back() == ' ') s.pop_back();
        out += "  " + s + "\n";
    }
    return out;
}

}  // namespace ll
