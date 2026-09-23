#include "ui/Renderer.h"

#include "common/Utf8.h"

namespace ll {
namespace {

// ANSI-цвета: янтарный «свет фонаря», серый для тьмы, красный для опасности.
const char* colorOf(MsgType type) {
    switch (type) {
        case MsgType::Text: return "";
        case MsgType::Dark: return "\x1b[90m";
        case MsgType::System: return "\x1b[37m";
        case MsgType::Hint: return "\x1b[2;37m";
        case MsgType::Title: return "\x1b[1;38;5;214m";
        case MsgType::Status: return "\x1b[38;5;180m";
        case MsgType::Damage: return "\x1b[91m";
        case MsgType::Heal: return "\x1b[92m";
        case MsgType::Item: return "\x1b[96m";
        case MsgType::Intent: return "\x1b[1;91m";
        case MsgType::Lore: return "\x1b[95m";
        case MsgType::Oil: return "\x1b[38;5;214m";
        case MsgType::Art: return "\x1b[38;5;223m";
        case MsgType::Map: return "\x1b[38;5;250m";
    }
    return "";
}

bool isPreformatted(MsgType type) {
    return type == MsgType::Art || type == MsgType::Map || type == MsgType::Status;
}

}  // namespace

std::string Renderer::render(const Message& message) const {
    if (message.text.empty()) return "\n";
    const std::vector<std::string> lines = isPreformatted(message.type)
                                               ? utf8::splitLines(message.text)
                                               : utf8::wrap(message.text, width_);
    const char* color = color_ ? colorOf(message.type) : "";
    std::string out;
    for (const std::string& line : lines) {
        if (*color != '\0') {
            out += color;
            out += line;
            out += "\x1b[0m";
        } else {
            out += line;
        }
        out += '\n';
    }
    return out;
}

void Renderer::flush(MessageLog& log, IConsole& console) const {
    std::string out;
    for (const Message& m : log.drain()) out += render(m);
    if (!out.empty()) console.write(out);
}

}  // namespace ll
