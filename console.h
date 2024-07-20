#ifndef CONSOLE_H
#define CONSOLE_H

#include "label.h"
#include "rect.h"

class Console : public GuiObject
{
public:
    Console(Renderer& renderer, float x, float y, float w, float h, const Font& font, std::move_only_function<void(const std::string&)>&& command_process_callback = {});

    virtual void update(Renderer& renderer) override;
    virtual void draw(Renderer& renderer) override;

    void print(std::string_view text);

    void onGotFocus() override;
    void onLostFocus() override;
    virtual void onKeyPressed(Key, const InputState&) override;
    virtual void onMousePressed(MouseButton, const InputState&) override;

    bool isPointInside(vec2) override;

    void setScissor(Quad) override;

private:
    float m_x, m_y, m_width, m_height;
    std::move_only_function<void(const std::string&)> m_command_process_callback;

    Rect m_rect;
    Label m_text_label;
    Label m_text_input;

    std::list<std::string> m_command_history;
    std::list<std::string>::iterator m_command_history_itr;
    static constexpr inline uint32_t m_max_command_history_size = 100;
    uint32_t m_command_history_size = 0;

    std::optional<Quad> m_scissor;
};

#endif // CONSOLE_H
