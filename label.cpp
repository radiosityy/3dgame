#include "label.h"
#include "game_utils.h"

Label::Label(Renderer& renderer, float x, float y, const Font& font, const std::string& text, bool editable, HorizontalAlignment hor_align, VerticalAlignment ver_align)
    : m_font(&font)
    , m_editable(editable)
    , m_reference_x(x)
    , m_reference_y(y)
    , m_horizontal_alignment(hor_align)
    , m_vertical_alignment(ver_align)
    , m_fixed_rect(false)
    , m_background_rect(renderer, 0.0f, 0.0f, 0.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f))
    , m_cursor_rect(renderer, 0.0f, 0.0f, 0.0f, 0.0f, ColorRGBA(0.2f, 0.2f, 0.5f, 1.0f))
{
    if(m_editable)
    {
        m_background_rect.setColor(ColorRGBA(0.3f, 0.3f, 0.3f, 1.0f));
    }

    setFocusable(m_editable);
    updateTextImpl(text);
}

Label::Label(Renderer& renderer, float x, float y, float width, float height, const Font& font, const std::string& text, bool editable, HorizontalAlignment hor_align, VerticalAlignment ver_align)
    : m_font(&font)
    , m_editable(editable)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_horizontal_alignment(hor_align)
    , m_vertical_alignment(ver_align)
    , m_fixed_rect(true)
    , m_background_rect(renderer, m_x, m_y, m_width, m_height, ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f))
    , m_cursor_rect(renderer, 0.0f, 0.0f, 0.0f, 0.0f, ColorRGBA(0.2f, 0.2f, 0.5f, 1.0f))
{
    if(m_editable)
    {
        m_background_rect.setColor(ColorRGBA(0.3f, 0.3f, 0.3f, 1.0f));
    }

    setFocusable(m_editable);

    switch(m_horizontal_alignment)
    {
    case HorizontalAlignment::Left:
        m_reference_x = m_x;
        break;
    case HorizontalAlignment::Center:
        m_reference_x = m_x + m_width * 0.5f;
        break;
    case HorizontalAlignment::Right:
        m_reference_x = m_x + m_width;
        break;
    default:
        error("Incorrect horizontal alignment!");
    }

    switch(m_vertical_alignment)
    {
    case VerticalAlignment::Top:
        m_reference_y = m_y;
        break;
    case VerticalAlignment::Center:
        m_reference_y = m_y + 0.5f * m_height;
        break;
    case VerticalAlignment::Bottom:
        m_reference_y = m_y + m_height;
        break;
    default:
        error("Incorrect vertical alignment!");
    }

    updateTextImpl(text);
}

void Label::onKeyPressed(Key key, const InputState& input_state)
{
    if(!m_editable)
    {
        return;
    }

    const bool uppercase = input_state.shift() != input_state.caps_lock;

    switch(key)
    {
    case VKeyLeft:
        if(m_cursor_position > 0)
        {
            updateCursor(m_cursor_position - 1);
        }
        break;
    case VKeyRight:
        if(m_cursor_position < text().size())
        {
            updateCursor(m_cursor_position + 1);
        }
        break;
    case VKeyHome:
        updateCursor(0);
        break;
    case VKeyEnd:
        updateCursor(text().size());
        break;
    case VKeyA:
        typeCharacter(uppercase ? "A" : "a");
        break;
    case VKeyB:
        typeCharacter(uppercase ? "B" : "b");
        break;
    case VKeyC:
        typeCharacter(uppercase ? "C" : "c");
        break;
    case VKeyD:
        typeCharacter(uppercase ? "D" : "d");
        break;
    case VKeyE:
        typeCharacter(uppercase ? "E" : "e");
        break;
    case VKeyF:
        typeCharacter(uppercase ? "F" : "f");
        break;
    case VKeyG:
        typeCharacter(uppercase ? "G" : "g");
        break;
    case VKeyH:
        typeCharacter(uppercase ? "H" : "h");
        break;
    case VKeyI:
        typeCharacter(uppercase ? "I" : "i");
        break;
    case VKeyJ:
        typeCharacter(uppercase ? "J" : "j");
        break;
    case VKeyK:
        typeCharacter(uppercase ? "K" : "k");
        break;
    case VKeyL:
        typeCharacter(uppercase ? "L" : "l");
        break;
    case VKeyM:
        typeCharacter(uppercase ? "M" : "m");
        break;
    case VKeyN:
        typeCharacter(uppercase ? "N" : "n");
        break;
    case VKeyO:
        typeCharacter(uppercase ? "O" : "o");
        break;
    case VKeyP:
        typeCharacter(uppercase ? "P" : "p");
        break;
    case VKeyQ:
        typeCharacter(uppercase ? "Q" : "q");
        break;
    case VKeyR:
        typeCharacter(uppercase ? "R" : "r");
        break;
    case VKeyS:
        typeCharacter(uppercase ? "S" : "s");
        break;
    case VKeyT:
        typeCharacter(uppercase ? "T" : "t");
        break;
    case VKeyU:
        typeCharacter(uppercase ? "U" : "u");
        break;
    case VKeyV:
        typeCharacter(uppercase ? "V" : "v");
        break;
    case VKeyW:
        typeCharacter(uppercase ? "W" : "w");
        break;
    case VKeyX:
        typeCharacter(uppercase ? "X" : "x");
        break;
    case VKeyY:
        typeCharacter(uppercase ? "Y" : "y");
        break;
    case VKeyZ:
        typeCharacter(uppercase ? "Z" : "z");
        break;
    case VKey1:
        typeCharacter(input_state.shift() ? "!" : "1");
        break;
    case VKey2:
        typeCharacter(input_state.shift() ? "@" : "2");
        break;
    case VKey3:
        typeCharacter(input_state.shift() ? "#" : "3");
        break;
    case VKey4:
        typeCharacter(input_state.shift() ? "$" : "4");
        break;
    case VKey5:
        typeCharacter(input_state.shift() ? "%" : "5");
        break;
    case VKey6:
        typeCharacter(input_state.shift() ? "^" : "6");
        break;
    case VKey7:
        typeCharacter(input_state.shift() ? "&" : "7");
        break;
    case VKey8:
        typeCharacter(input_state.shift() ? "*" : "8");
        break;
    case VKey9:
        typeCharacter(input_state.shift() ? "(" : "9");
        break;
    case VKey0:
        typeCharacter(input_state.shift() ? ")" : "0");
        break;
    case VKeyColon:
        typeCharacter(input_state.shift() ? ":" : ";");
        break;
    case VKeyLBracket:
        typeCharacter(input_state.shift() ? "{" : "[");
        break;
    case VKeyRBracket:
        typeCharacter(input_state.shift() ? "}" : "]");
        break;
    case VKeyQuote:
        typeCharacter(input_state.shift() ? "'" : "\"");
        break;
    case VKeyComma:
        typeCharacter(input_state.shift() ? "<" : ",");
        break;
    case VKeyPeriod:
        typeCharacter(input_state.shift() ? ">" : ".");
        break;
    case VKeyMinus:
        typeCharacter(input_state.shift() ? "_" : "-");
        break;
    case VKeyPlus:
        typeCharacter(input_state.shift() ? "+" : "=");
        break;
    case VKeySlash:
        typeCharacter(input_state.shift() ? "?" : "/");
        break;
    case VKeyBackslash:
        typeCharacter(input_state.shift() ? "|" : "\\");
        break;
    case VKeySpace:
        typeCharacter(" ");
        break;
    case VKeyBackspace:
        if((m_cursor_position > 0) && !m_text.empty())
        {
            const size_t pos_from_back = m_text.size() - m_cursor_position;

            auto text = m_text;
            text.erase(m_cursor_position - 1, 1);
            updateText(text);
            updateCursor(m_text.size() - pos_from_back);
        }
        break;
    case VKeyDelete:
        if((m_cursor_position < text().size()) && !m_text.empty())
        {
            auto text = m_text;
            text.erase(m_cursor_position, 1);
            updateText(text);
            updateCursor(m_cursor_position);
        }
        break;
    case VKeyEnter:
        if(m_confirm_callback)
        {
            confirm();
        }
        else if(m_multiline)
        {
            updateText(m_text + '\n');
        }
        break;
    case VKeyEsc:
        cancel();
        break;
    }
}

void Label::onMousePressed(MouseButton mb, const InputState& input_state)
{
    if(LMB != mb)
    {
        return;
    }

    if(m_editable && !m_text.empty())
    {
        float base_x = m_base_x;

        //first check if cursor is to the left of the first glyph (if it is we can immediately return)
        {
            const float start_x = base_x;
            if(input_state.cursor_pos.x < std::truncf(start_x))
            {
                return;
            }
        }

        //then loop over consecutive characters and check if the cursor is to the left of any of them
        //if it is then we can select the previous glyph
        for(size_t i = 1; i < m_text.size(); i++)
        {
            const auto& prev_g = m_font->glyphInfo(m_text[i - 1]);

            base_x += prev_g.advance + prev_g.kerning[m_text[i]];

            if(input_state.cursor_pos.x < std::truncf(base_x))
            {
                updateCursor(i - 1);
                return;
            }
        }

        //finally, if we got here, check if cursor is to the left of the end of the last glyph
        const auto& g = m_font->glyphInfo(m_text.back());
        const float end_x = base_x + g.advance;
        if(input_state.cursor_pos.x < std::truncf(end_x))
        {
            updateCursor(m_text.size() - 1);
            return;
        }

        //if we click completely to the right of the text then put the label cursor past the end of the text
        updateCursor(m_text.size());
    }
}

void Label::setTextChangedCallback(std::move_only_function<void (std::string_view)>&& callback)
{
    m_text_changed_callback = std::move(callback);
}

void Label::update(Renderer& renderer)
{
    if(m_vertex_data_changed)
    {
        m_vertex_data_changed = false;

        updateVertexData();

        //TODO: maybe alloc vertex count should be in the VertexBufferAllocation structure?
        if(m_vertices.size() * sizeof(VertexUi) > m_vb_alloc.size)
        {
            if(m_vb_alloc.size > 0)
            {
                renderer.freeVertexBufferAllocation(m_vb_alloc);
            }

            m_vb_alloc = renderer.requestVertexBufferAllocation<VertexUi>(m_vertices.size());
        }

        if(!m_vertices.empty())
        {
            renderer.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexUi) * m_vertices.size(), m_vertices.data());
        }
    }

    m_background_rect.update(renderer);
    if(m_focused)
    {
        m_cursor_rect.update(renderer);
    }
}

void Label::draw(Renderer& renderer)
{
    m_background_rect.draw(renderer);
    if(m_focused)
    {
        m_cursor_rect.draw(renderer);
    }

    if(!m_vertices.empty())
    {
        renderer.drawUi(RenderModeUi::Font, m_vb_alloc.vb, m_vb_alloc.vertex_offset, m_vertices.size(), m_scissor);
    }
}

void Label::setBackgroundColor(ColorRGBA color)
{
    m_background_rect.setColor(color);
}

void Label::setBackgroundTex(TexId tex)
{
    m_background_rect.setTexture(tex);
}

float Label::height() const
{
    return m_height;
}

float Label::width() const
{
    return m_width;
}

void Label::setText(std::string_view text)
{
    if(!m_focused || m_enable_set_text_when_focused)
    {
        updateText(text);
        updateCursor(m_text.size());
    }
}

void Label::appendText(std::string_view text)
{
    setText(m_text + text.data());
}

bool Label::isPointInside(vec2 p)
{
    return m_background_rect.isPointInside(p);
}

void Label::setScissor(Quad scissor)
{
    m_parent_scissor = scissor;
    updateScissors();
}

const std::string& Label::text()
{
    return m_text;
}

void Label::move(vec2 xy)
{
    m_x += xy.x;
    m_reference_x += xy.x;
    m_base_x += xy.x;

    m_y += xy.y;
    m_reference_y += xy.y;

    setText(m_text);
}

void Label::setConfirmCallback(std::move_only_function<void (Label&)>&& confirm_callback, bool clear_on_confirm)
{
    m_confirm_callback = std::move(confirm_callback);
    m_clear_on_confirm = clear_on_confirm;
    m_multiline = false;
}

void Label::setCancalable(bool cancelable)
{
    m_cancelable = cancelable;
}

void Label::setMultiline(bool multiline)
{
    m_multiline = multiline;
}

void Label::setConfirmOnFocusLost(bool confirm_on_focus_lost) noexcept
{
    m_confirm_on_focus_lost = confirm_on_focus_lost;
}

void Label::enableSetTextWhenFocued(bool enable_text_update_when_focused) noexcept
{
    m_enable_set_text_when_focused = enable_text_update_when_focused;
}

void Label::onGotFocus()
{
    m_focused = true;
    m_prev_text = text();
    updateCursor(m_text.size());
}

void Label::onLostFocus()
{
    m_focused = false;

    if(m_confirm_on_focus_lost)
    {
        confirm();
    }
    else
    {
        cancel();
    }
}

void Label::updateText(std::string_view text)
{
    if(m_text != text)
    {
        updateTextImpl(text);
    }
}

void Label::updateTextImpl(std::string_view text)
{
    m_text = text;
    updateVertexData();

    if(m_text_changed_callback)
    {
        m_text_changed_callback(text);
    }
}

void Label::updateVertexData()
{
    //TODO: don't do all this if text is empty, but some stuff still needs to be done
    m_vertices.clear();
    m_vertices.reserve(m_text.size());

    const auto line_measurements = m_font->getMeasurments(m_text);

    const auto baseX = [&](size_t line_id)
    {
        switch(m_horizontal_alignment)
        {
        case HorizontalAlignment::Left:
            return m_reference_x;
        case HorizontalAlignment::Center:
            return m_reference_x - line_measurements[line_id].x * 0.5f;
        case HorizontalAlignment::Right:
            return m_reference_x - line_measurements[line_id].x;
        default:
            error("Incorrect horizontal alignment!");
        }
    };

    const float height = (m_font->height() + (line_measurements.size() - 1) * m_font->baselineDistance());

    float y;

    switch(m_vertical_alignment)
    {
    case VerticalAlignment::Top:
        y = m_reference_y;
        break;
    case VerticalAlignment::Center:
        y = m_reference_y - 0.5f * height;
        break;
    case VerticalAlignment::Bottom:
        y = m_reference_y - height;
        break;
    default:
        error("Incorrect vertical alignment!");
    }

    m_base_x = baseX(0);

    if(!m_fixed_rect)
    {
        m_x = baseX(0);
        m_width = line_measurements[0].x;
        for(uint32_t line_id = 1; line_id < line_measurements.size(); line_id++)
        {
            m_x = std::min(m_x, baseX(line_id));
            m_width = std::max(m_width, static_cast<float>(line_measurements[line_id].x));
        }

        m_y = y;
        m_height = height;
    }


    const float w = m_font->texWidth();
    const float h = m_font->texHeight();
    auto x = baseX(0);
    size_t line_id = 0;

    for(size_t i = 0; i < m_text.size(); i++)
    {
        const auto c = m_text[i];

        if('\n' == c)
        {
            line_id++;
            x = baseX(line_id);
            y += m_font->baselineDistance();
            continue;
        }

        const auto& g = m_font->glyphInfo(c);
        const float a = g.advance;
        const float k = (i < m_vertices.size() - 1) ? g.kerning[m_text[i+1]] : 0;

        if(' ' == c)
        {
            x += a + k;
            continue;
        }

        m_vertices.emplace_back();
        auto& v = m_vertices.back();

        const float bx = g.bearing_x;
        const float d = g.descent;

        v.tex_id = 0;
        v.layer_id = m_font->glyphInfo(c).tex_id;
        v.top_left_pos = vec2(std::truncf(x + bx), std::truncf(y + d));
        v.size = vec2(w, h);

        x += a + k;
    }

    if(!m_fixed_rect)
    {
        m_background_rect.setPosAndSize(m_x, m_y, m_width, m_height);
    }

    updateScissors();
    m_vertex_data_changed = true;
}

void Label::updateScissors()
{
    if(m_parent_scissor)
    {
        m_scissor = quadOverlap({m_x, m_y, m_width, m_height}, *m_parent_scissor);
    }
    else
    {
        m_scissor = {m_x, m_y, m_width, m_height};
    }

    m_background_rect.setScissor(m_scissor);
    m_cursor_rect.setScissor(m_scissor);
}

void Label::typeCharacter(const char* c)
{
    auto text = m_text;
    text.insert(m_cursor_position, c);
    updateText(text);
    updateCursor(m_cursor_position + 1);
}

void Label::updateCursor(size_t new_pos)
{
    if(!m_focused)
    {
        return;
    }

    m_cursor_position = new_pos;

    float x = m_base_x + m_font->getMeasurments(text().substr(0, m_cursor_position))[0].x;
    float char_width;

    if(m_cursor_position < text().size())
    {
        const auto& g = m_font->glyphInfo(m_text[m_cursor_position]);
        const float k = (m_cursor_position < m_text.size() - 1) ? g.kerning[m_text[m_cursor_position + 1]] : 0;
        char_width = g.advance + k;
    }
    else
    {
        char_width = m_font->getMeasurments("W")[0].x;
    }

    m_cursor_rect.setPosAndSize(x, m_y, char_width, m_height);
}

void Label::cancel()
{
    if(m_cancelable)
    {
        updateText(m_prev_text);
        updateCursor(this->text().size());
    }
}

void Label::confirm()
{
    m_confirm_callback(*this);
    if(m_clear_on_confirm)
    {
        updateText("");
        updateCursor(0);
    }
    if(m_cancelable)
    {
        m_prev_text = text();
    }
}
