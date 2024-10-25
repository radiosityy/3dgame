#ifndef LABEL_H
#define LABEL_H

#include "gui_object.h"
#include "font.h"
#include "rect.h"

class Label : public GuiObject
{
public:
    Label(Renderer& renderer, float x, float y, const Font& font, const std::string& text, bool editable = false, HorizontalAlignment = HorizontalAlignment::Left, VerticalAlignment = VerticalAlignment::Top);
    Label(Renderer& renderer, float x, float y, float width, float height, const Font& font, const std::string& text, bool editable = false, HorizontalAlignment = HorizontalAlignment::Center, VerticalAlignment = VerticalAlignment::Center);

    virtual void onKeyPressed(Key, const InputState&) override;
    virtual void onMousePressed(MouseButton, const InputState&) override;

    virtual void setTextChangedCallback(std::move_only_function<void(std::string_view)>&&);
    virtual void update(Renderer& renderer) override;
    virtual void draw(Renderer& renderer) override;

    void setBackgroundColor(ColorRGBA color);
    void setBackgroundTex(TexId tex);

    float height() const;
    float width() const;

    void setText(std::string_view text);
    void appendText(std::string_view text);

    virtual bool isPointInside(vec2) override;

    virtual void setScissor(Quad) override;

    const std::string& text();

    void setRect(float x, float y, float width, float height);
    virtual void move(vec2) override;

    void setConfirmCallback(std::move_only_function<void(Label&)>&& confirm_callback, bool clear_on_confirm = false);
    void setCancalable(bool);
    void setMultiline(bool);

    void setConfirmOnFocusLost(bool) noexcept;
    void enableSetTextWhenFocued(bool) noexcept;

    void onGotFocus() override;
    void onLostFocus() override;

private:
    void updateText(std::string_view text);
    void updateTextImpl(std::string_view text);
    void updateVertexData();
    void updateScissors();
    void updateReferenceXY();

    const Font* m_font = nullptr;
    std::string m_text;

    const bool m_editable = false;

    float m_reference_x = 0.0f;
    float m_reference_y = 0.0f;
    float m_base_x = 0.0f;

    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 0.0f;
    float m_height = 0.0f;
    HorizontalAlignment m_horizontal_alignment;
    VerticalAlignment m_vertical_alignment;

    std::vector<VertexUi> m_vertices;

    bool m_fixed_rect = false;
    Rect m_background_rect;

    std::optional<Quad> m_parent_scissor;
    Quad m_scissor;

    /*text input*/
    void typeCharacter(const char*);
    void updateCursor(size_t new_pos);
    void cancel();
    void confirm();

    Rect m_cursor_rect;

    std::move_only_function<void(Label&)> m_confirm_callback;
    std::move_only_function<void(std::string_view)> m_text_changed_callback;
    bool m_cancelable = false;
    bool m_multiline = false;
    bool m_clear_on_confirm = false;
    bool m_confirm_on_focus_lost = false;
    bool m_enable_set_text_when_focused = false;

    size_t m_cursor_position = 0;
    std::string m_prev_text;

    bool m_focused = false;

    VertexBufferAllocation m_vb_alloc;
    bool m_vertex_data_changed = false;
};

#endif // LABEL_H
