#include "console.h"
#include <format>

Console::Console(Engine3D& engine3d, float x, float y, float w, float h, const Font& font, std::move_only_function<void(const std::string&)>&& command_process_callback)
    : m_x(x)
    , m_y(y)
    , m_width(w)
    , m_height(h)
    , m_command_process_callback(std::move(command_process_callback))
    , m_rect(engine3d, m_x, m_y, m_width, 0.9f * m_height, ColorRGBA(0.2f, 0.2f, 0.2f, 1.0f))
    , m_text_label(engine3d, m_x, m_y + 0.9f * m_height, font, "", false, HorizontalAlignment::Left, VerticalAlignment::Bottom)
    , m_text_input(engine3d, m_x, m_y + 0.9f * m_height, m_width, 0.1f * m_height, font, "", true, HorizontalAlignment::Left, VerticalAlignment::Center)
{
    m_text_input.setConfirmCallback([&](Label& text_input)
    {
        if(m_command_history_size == m_max_command_history_size)
        {
            m_command_history.pop_back();
        }
        else
        {
            m_command_history_size++;
        }

        m_command_process_callback(text_input.text());

        m_command_history.front() = text_input.text();
        text_input.setText("");

        m_command_history.push_front("");
        m_command_history_itr = m_command_history.begin();
    });

    m_command_history.push_front("");
    m_command_history_itr = m_command_history.begin();
    m_command_history_size = 1;

    m_text_label.setScissor({m_x, m_y, m_width, m_height});
    m_text_input.setScissor({m_x, m_y, m_width, m_height});
}

void Console::update(Engine3D& engine3d, float dt)
{
    m_rect.update(engine3d, dt);
    m_text_label.update(engine3d, dt);
    m_text_input.update(engine3d, dt);
}

void Console::draw(Engine3D& engine3d)
{
    m_rect.draw(engine3d);
    m_text_label.draw(engine3d);
    m_text_input.draw(engine3d);
}

void Console::print(std::string_view text)
{
    if(m_text_label.text().empty())
    {
        m_text_label.appendText(text);
    }
    else
    {
        m_text_label.appendText(std::format("\n{}", text));
    }
}

void Console::gotFocus()
{
    m_text_input.gotFocus();
}

void Console::lostFocus()
{
    m_text_input.lostFocus();
}

void Console::onInputEvent(const Event& event, const InputState& input_state)
{
    if(event.event == EventType::KeyPressed)
    {
        if(event.key == VKeyUp)
        {
            if(auto prev = std::next(m_command_history_itr); prev != m_command_history.end())
            {
                *m_command_history_itr = m_text_input.text();
                m_command_history_itr = prev;
                m_text_input.setText(*m_command_history_itr);
            }
            return;
        }
        else if(event.key == VKeyDown)
        {
            if(m_command_history_itr != m_command_history.begin())
            {
                *m_command_history_itr = m_text_input.text();
                m_command_history_itr--;
                m_text_input.setText(*m_command_history_itr);
            }
            return;
        }
    }

    m_text_input.onInputEvent(event, input_state);
}

bool Console::isPointInside(vec2 p)
{
    const auto d = p - vec2(m_x, m_y);
    return (d.x > 0) && (d.x <= m_width) && (d.y > 0) && (d.y <= m_height);
}

void Console::setScissor(Quad scissor)
{
    m_scissor = scissor;

    const auto min_scissor = quadOverlap({m_x, m_y, m_width, m_height}, scissor);

    m_text_label.setScissor(min_scissor);
    m_text_input.setScissor(min_scissor);
}
