#ifndef EDITOR_H
#define EDITOR_H

#include "gui_object.h"
#include "window.h"
#include "scene.h"
#include "rect.h"
#include "point_light_edit_panel.h"
#include "object_add_panel.h"

class Editor : public GuiParent
{
    enum class Mode
    {
        None,
        Move,
        Scale,
        Rotate
    };

    enum class Axis
    {
        None = 0,
        X = 1,
        Y = 2,
        Z = 4
    };

public:
    Editor(Window& window, Scene& scene, Engine3D& engine3d, const Font& font);

    void update(const InputState& input_state, float dt);
    void draw();
    void onWindowResize(uint32_t width, uint32_t height, float scale_x, float scale_y);
    void onInputEvent(const Event& event, const InputState& input_state);

    vec4 highlightColor() const;
    void curTerrainPos(bool& cur_terrain_intersection, vec3& cur_terrain_pos) const;
    void terrainToolRadii(float& inner, float& outer);

private:
    PointLight& selectedPointLight();

    void openPointLightEditPanel(PointLightId point_light_id);
    void closePointLightEditPanel();
    void updateSelectedPointLight();

    void moveModeHandleEvent(const Event& event, const InputState& input_state);
    void scaleModeHandleEvent(const Event& event, const InputState& input_state);
    void rotModeHandleEvent(const Event& event, const InputState& input_state);

    void deselectAll();

    Scene& m_scene;
    Window& m_window;
    Engine3D& m_engine3d;
    const Font& m_font;

    Object* m_selected_object = nullptr;
    std::optional<uint32_t> m_selected_point_light_id;

    std::vector<VertexQuad> m_vertex_quad_data;
    VertexBufferAllocation m_billboard_vb_alloc;

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

    std::vector<std::unique_ptr<GuiObject>> m_gui_objects;

    size_t m_point_light_edit_panel_id;
    ObjectAddPanel* m_object_add_panel = nullptr;

    static constexpr float m_terrain_tool_width = 0.5f;
    float m_terrain_tool_radius = 10.0f;
    bool m_cur_terrain_intersection = false;
    vec3 m_cur_terrain_pos;

};

#endif // EDITOR_H
