#include "common/MessageLog.h"

#include <utility>

namespace ll {

void MessageLog::push(MsgType type, std::string text) {
    messages_.push_back({type, std::move(text)});
}

void MessageLog::blank() {
    // Две пустые строки подряд не нужны.
    if (!messages_.empty() && messages_.back().text.empty()) return;
    messages_.push_back({MsgType::Text, {}});
}

std::vector<Message> MessageLog::drain() {
    std::vector<Message> out;
    out.swap(messages_);
    return out;
}

std::string MessageLog::joinedText() const {
    std::string out;
    for (const auto& m : messages_) {
        out += m.text;
        out += '\n';
    }
    return out;
}

}  // namespace ll
