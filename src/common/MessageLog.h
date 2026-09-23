#pragma once

#include <string>
#include <vector>

#include "common/Enums.h"

namespace ll {

struct Message {
    MsgType type = MsgType::Text;
    std::string text;
};

// Буфер сообщений: игровая логика пишет сюда, а печатает только Renderer.
class MessageLog {
public:
    void push(MsgType type, std::string text);
    void blank();  // пустая строка-разделитель
    std::vector<Message> drain();
    const std::vector<Message>& peek() const { return messages_; }
    bool empty() const { return messages_.empty(); }
    std::string joinedText() const;  // весь текст подряд — удобно для тестов

private:
    std::vector<Message> messages_;
};

}  // namespace ll
