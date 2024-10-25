#ifndef EDITOR_H
#define EDITOR_H

#include "gui_object.h"
#include "window.h"
#include "scene.h"
#include "point_light_edit_panel.h"
#include "object_add_panel.h"

class Editor : public GuiParent
{
    enum class Mode
    {
        None,
        Move,
        Scale,
        Rotate,
        Terrain
    };

    enum class Axis
    {
        None = 0,
        X = 1,
        Y = 2,
        Z = 4
    };

public:
    Editor(uint32_t window_width, uint32_t window_height, Scene& scene, Renderer& renderer, const Font& font);
    //TODO: this destructor declaration is not needed?
    virtual ~Editor() = default;

    void update(const InputState& input_state, float dt, bool process_input);
    void draw(RenderData&);
    void onWindowResize(uint32_t width, uint32_t height);

    virtual bool onKeyPressedIntercept(Key, const InputState&) override;

    virtual void onKeyPressedImpl(Key, const InputState&) override;
    virtual void onMousePressedImpl(MouseButton, const InputState&) override;
    virtual void onMouseMovedImpl(vec2, const InputState&) override;
    virtual void onMouseScrolledUpImpl(const InputState&) override;
    virtual void onMouseScrolledDownImpl(const InputState&) override;

private:
    PointLight& selectedPointLight();

    void openObjectAddPanel();
    void closeObjectAddPanel();
    void openPointLightEditPanel(PointLightId point_light_id);
    void closePointLightEditPanel();
    void updateSelectedPointLight();
    void updatePointLightBillboards();

    void deselectAll();
    void toggleAxis(Axis);
    void confirmTransform();
    void cancelTransform();

    Scene& m_scene;
    uint32_t m_window_width = 0;
    uint32_t m_window_height = 0;
    Renderer& m_renderer;
    const Font& m_font;

    Object* m_selected_object = nullptr;
    std::optional<uint32_t> m_selected_point_light_id;

    std::vector<VertexBillboard> m_vertex_billboard_data;
    VertexBufferAllocation m_billboard_vb_alloc;

    bool m_render_point_light_billboards = true;
    bool m_point_light_billboards_in_front = true;

    static constexpr float m_selected_object_blink_time = 2.0f;
    float m_selected_object_blink_elapsed_time = 0.0f;
    static constexpr float m_selected_object_max_alpha = 0.1f;
    float m_selected_object_alpha = 0.05f;

    Mode m_mode = Mode::None;
    Axis m_axis_lock = Axis::None;
    vec3 m_original_pos;
    vec3 m_original_scale;
    quat m_original_rot;
    vec2 m_rot_cursor_pos;

    static constexpr float m_billboard_size_x = 2.0f;
    static constexpr float m_billboard_size_y = 2.0f;

    ObjectAddPanel* m_object_add_panel = nullptr;
    PointLightEditPanel* m_point_light_edit_panel = nullptr;

    static constexpr float m_terrain_tool_width = 0.5f;
    static constexpr float m_min_terrain_tool_radius = 1.0f;
    float m_terrain_tool_radius = 10.0f;
    bool m_cur_terrain_intersection = false;
    vec3 m_cur_terrain_pos;

};

#endif // EDITOR_H
