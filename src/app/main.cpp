#include "common/MessageLog.h"
#include "ui/IConsole.h"
#include "ui/Renderer.h"

// Шаг 1: каркас — консоль с кириллицей, цвета и эхо ввода.
int main() {
    auto console = ll::makeSystemConsole();
    ll::Renderer renderer(console->supportsColor(), 78);
    ll::MessageLog log;
    log.push(ll::MsgType::Title, "П О С Л Е Д Н И Й   Ф О Н А Р Щ И К");
    log.push(ll::MsgType::Dark, "Каркас игры. Введите строку (пустая строка — выход).");
    for (;;) {
        renderer.flush(log, *console);
        console->write("> ");
        auto line = console->readLine();
        if (!line || line->empty()) break;
        log.push(ll::MsgType::Oil, "Вы ввели: " + *line);
    }
    return 0;
}
