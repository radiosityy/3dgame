#ifndef LABEL_H
#define LABEL_H

#include "gui_object.h"
#include "font.h"
#include "rect.h"

class Label : public GuiObject
{
public:
    enum class Action {None, Confirm, Cancel};

    Label(Engine3D& engine3d, float x, float y, const Font& font, const std::string& text, bool editable = false, HorizontalAlignment = HorizontalAlignment::Left, VerticalAlignment = VerticalAlignment::Top);
    Label(Engine3D& engine3d, float x, float y, float width, float height, const Font& font, const std::string& text, bool editable = false, HorizontalAlignment = HorizontalAlignment::Center, VerticalAlignment = VerticalAlignment::Center);

    virtual void onInputEvent(const Event& event, const InputState& input_state) override;
    virtual void onResolutionChange(float scale_x, float scale_y, const Font& font) override;

    virtual void setUpdateCallback(std::function<void(Label&)>&&);
    virtual void setTextChangedCallback(std::function<void(std::string_view)>&&);
    virtual void update(Engine3D& engine3d, float dt) override;
    virtual void draw(Engine3D& engine3d) override;

    void setBackgroundColor(ColorRGBA color);
    void setBackgroundTex(TexId tex);

    float height() const;
    float width() const;

    void setText(const std::string& text);
    void appendText(const std::string& text);

    virtual bool isPointInside(vec2) override;

    virtual void setScissor(Quad) override;

    const std::string& text();

    virtual void move(vec2) override;

    void setConfirmCallback(std::function<void(Label&)>&&);
    void setCancalable(bool);
    void setMultiline(bool);

    void setActionOnFocusLost(Action);

    void gotFocus() override;
    void lostFocus() override;

private:
    void updateVertexData();
    void updateScissors();
    void setTextNoCursorUpdate(const std::string& text);

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

    std::function<void(Label&)> m_update_callback;

    std::optional<Quad> m_parent_scissor;
    Quad m_scissor;

    /*text input*/
    void typeCharacter(const char*);
    void updateCursor(size_t new_pos);
    void cancel();

    Rect m_cursor_rect;

    std::function<void(Label&)> m_confirm_callback;
    std::function<void(std::string_view)> m_text_changed_callback;
    bool m_cancelable = false;
    bool m_multiline = false;
    Action m_action_on_focus_lost = Action::Cancel;

    size_t m_cursor_position = 0;
    std::string m_prev_text;

    bool m_focused = false;

    VertexBufferAllocation m_vb_alloc;
    uint32_t m_vb_alloc_vertex_count = 0;
    bool m_vertex_data_updated = false;
};

#endif // LABEL_H
