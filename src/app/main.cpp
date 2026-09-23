#include <filesystem>
#include <string>

#include "common/MessageLog.h"
#include "data/DataLoader.h"
#include "data/DataValidator.h"
#include "model/WorldFactory.h"
#include "ui/IConsole.h"
#include "ui/Renderer.h"

namespace fs = std::filesystem;

// Шаг 2: загрузка и проверка данных, построение начального состояния мира.
int main(int argc, char** argv) {
    auto console = ll::makeSystemConsole();
    ll::Renderer renderer(console->supportsColor(), 78);
    ll::MessageLog log;

    fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    try {
        ll::DataLoader loader(root);
        ll::GameData data = loader.loadAll();
        const auto errors = ll::DataValidator().validate(data);
        if (!errors.empty()) {
            log.push(ll::MsgType::Damage, "Ошибки в данных игры:");
            for (const auto& e : errors) log.push(ll::MsgType::Damage, "  " + e);
            renderer.flush(log, *console);
            return 1;
        }
        ll::WorldState world = ll::WorldFactory::create(data);
        log.push(ll::MsgType::Art, data.texts.get(data.config.texts.title));
        log.push(ll::MsgType::System,
                 "Загружено: комнат " + std::to_string(data.rooms.size()) + ", предметов " +
                     std::to_string(data.items.size()) + ", врагов " + std::to_string(data.enemies.size()) +
                     ", текстов " + std::to_string(data.texts.size()));
        const ll::RoomDef& start = data.room(world.player.location);
        log.push(ll::MsgType::Title, "── " + start.name + " ──");
        log.push(ll::MsgType::Text, start.description);
    } catch (const ll::DataError& e) {
        log.push(ll::MsgType::Damage, std::string("Ошибка загрузки данных: ") + e.what());
        renderer.flush(log, *console);
        return 1;
    }
    renderer.flush(log, *console);
    return 0;
}
