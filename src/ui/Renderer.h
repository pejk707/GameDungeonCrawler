#pragma once

#include <cstddef>
#include <string>

#include "common/MessageLog.h"
#include "ui/IConsole.h"

namespace ll {

// Единственное место, где сообщения превращаются в текст на экране:
// цвет по типу сообщения и перенос строк по ширине.
class Renderer {
public:
    Renderer(bool color, std::size_t width) : color_(color), width_(width) {}

    void flush(MessageLog& log, IConsole& console) const;
    std::string render(const Message& message) const;

    void setColor(bool color) { color_ = color; }
    bool color() const { return color_; }
    std::size_t width() const { return width_; }

private:
    bool color_;
    std::size_t width_;
};

}  // namespace ll
