#if EDITOR_ENABLE
#include "editor.h"
#include <iostream>

Editor::Editor(Window& window, Scene& scene, Engine3D& engine3d, const Font& font)
    : m_scene(scene)
    , m_window(window)
    , m_engine3d(engine3d)
    , m_font(font)
{
    auto object_add_panel = std::make_unique<ObjectAddPanel>(engine3d, 400.0f, 400.0f, scene, font);
    object_add_panel->setVisible(false);
    m_object_add_panel = object_add_panel.get();
    m_gui_objects.push_back(std::move(object_add_panel));

    m_billboard_vb_alloc = m_engine3d.requestVertexBufferAllocation<VertexQuad>(MAX_POINT_LIGHT_COUNT);

    updatePointLightBillboards();
}

void Editor::update(const InputState& input_state, float dt)
{
    if(Mode::Terrain == m_mode)
    {
        const vec2 cur_pos_ndc(2.0f * (m_window.inputState().cursor_pos.x / m_window.width()) - 1.0f, -2.0f * (m_window.inputState().cursor_pos.y / m_window.height()) + 1.0f);
        Ray ray(m_scene.camera().pos(), m_scene.camera().cursorProjW(cur_pos_ndc));
        float d;

        m_cur_terrain_intersection = m_scene.terrain().rayIntersection(ray, d);

        if(m_cur_terrain_intersection)
        {
            m_cur_terrain_pos = ray.origin + d * ray.dir;
        }

        if(m_cur_terrain_intersection && (input_state.mouse & LMB))
        {
            m_scene.terrain().toolEdit(m_engine3d, m_cur_terrain_pos, m_terrain_tool_radius, 0.05f);
        }
    }
    else if(Mode::Transform == m_mode)
    {
        if(m_selected_object)
        {
            m_selected_object_blink_elapsed_time += dt;

            if(m_selected_object_blink_elapsed_time >= m_selected_object_blink_time)
            {
                m_selected_object_blink_elapsed_time -= m_selected_object_blink_time;
            }

            if(m_selected_object_blink_elapsed_time <= 0.5f * m_selected_object_blink_time)
            {
                m_selected_object_alpha = m_selected_object_max_alpha * m_selected_object_blink_elapsed_time / (0.5f * m_selected_object_blink_time);
            }
            else
            {
                m_selected_object_alpha = m_selected_object_max_alpha * (1.0f - (m_selected_object_blink_elapsed_time - 0.5f * m_selected_object_blink_time) / (0.5f * m_selected_object_blink_time));
            }
        }
    }

    if(!m_keyboard_focus)
    {
        /*camera movement*/
        Camera& camera = m_scene.camera();
        const float cam_speed = input_state.shift() ? 42.0f : 14.0f;

        if(input_state.keyboard[VKeyW] && !input_state.ctrl())
        {
            camera.walk(dt*cam_speed);
        }

        if(input_state.keyboard[VKeyS] && !input_state.ctrl())
        {
            camera.walk(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeyA] && !input_state.ctrl())
        {
            camera.strafe(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeyD] && !input_state.ctrl())
        {
            camera.strafe(dt*cam_speed);
        }

        if(input_state.keyboard[VKeyC] && !input_state.ctrl())
        {
            camera.tilt(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeySpace] && !input_state.ctrl())
        {
            camera.tilt(dt*cam_speed);
        }
    }

    for(auto& child : m_gui_objects)
    {
        //TODO: handle visibility properly, also outside of editor (same as in Editor::draw())
        if(child->isVisible())
        {
            child->update(m_engine3d, dt);
        }
    }
}

void Editor::draw()
{
    /*selected object highlight*/
    if(Mode::Transform == m_mode)
    {
        if(m_selected_object)
        {
            m_selected_object->drawHighlight(m_engine3d);
        }
    }

    /*point light billboards*/
    m_engine3d.draw(RenderMode::Billboard, m_billboard_vb_alloc.vb, m_billboard_vb_alloc.vertex_offset, m_vertex_quad_data.size(), 0, {}, {});

    for(auto& child : m_gui_objects)
    {
        //TODO: handle visibility properly, also outside of editor
        if(child->isVisible())
        {
            child->draw(m_engine3d);
        }
    }
}

void Editor::onWindowResize(uint32_t width, uint32_t height, float scale_x, float scale_y)
{

}

void Editor::onInputEvent(const Event& event, const InputState& input_state)
{
    determineFocus(m_gui_objects, event, input_state);
    if(forwardEventToFocused(event, input_state))
    {
        return;
    }

    if(event.event == EventType::MouseDragged)
    {
        if(input_state.mouse & MMB)
        {
            m_scene.camera().rotate(event.cursor_delta.x * 0.005f);
            m_scene.camera().pitch(event.cursor_delta.y * 0.005f);
        }
    }

    if((Mode::None == m_mode) || ((Mode::Transform == m_mode) && (TransformMode::None == m_transform_mode)))
    {
        if((event.event == EventType::MousePressed) && (event.mouse == LMB))
        {
            deselectAll();

            const float aspect_ratio = m_scene.camera().aspectRatio();
            const float dx = 2.0f * aspect_ratio / m_window.width();
            const float dy = 2.0f / m_window.height();

            const vec3 ray_dirV = vec3(-aspect_ratio + dx * (0.5f + input_state.cursor_pos.x),
                                       1.0f - dy * (0.5f + input_state.cursor_pos.y),
                                       m_scene.camera().imagePlaneDistance());

            Ray ray(m_scene.camera().pos(), m_scene.camera().invView() * vec4(ray_dirV, 0.0f));
            float min_d = std::numeric_limits<float>::max();

            if(m_render_point_light_billboards)
            {
                /*for every point light we test ray intersection with the 2 triangles
                that compose the point light billboard in editor*/
                for(uint32_t i = 0; i < m_scene.staticPointLights().size(); i++)
                {
                    const auto& point_light = m_scene.staticPointLights()[i];

                    float d;

                    vec3 p0 = point_light.pos + vec3(-0.5f * m_billboard_size_x, 0.5f * m_billboard_size_y, 0.0f);
                    vec3 p1 = point_light.pos + vec3(0.5f * m_billboard_size_x, 0.5f * m_billboard_size_y, 0.0f);
                    vec3 p2 = point_light.pos + vec3(-0.5f * m_billboard_size_x, -0.5f * m_billboard_size_y, 0.0f);

                    if(intersect(ray, p0, p1, p2, d))
                    {
                        if(d < min_d)
                        {
                            min_d = d;
                            m_selected_point_light_id = i;
                            continue;
                        }
                    }

                    p0 = point_light.pos + vec3(-0.5f * m_billboard_size_x, -0.5f * m_billboard_size_y, 0.0f);
                    p1 = point_light.pos + vec3(0.5f * m_billboard_size_x, 0.5f * m_billboard_size_y, 0.0f);
                    p2 = point_light.pos + vec3(0.5f * m_billboard_size_x, -0.5f * m_billboard_size_y, 0.0f);

                    if(intersect(ray, p0, p1, p2, d))
                    {
                        if(d < min_d)
                        {
                            min_d = d;
                            m_selected_point_light_id = i;
                            continue;
                        }
                    }
                }
            }

            if(!m_selected_point_light_id || !m_point_light_billboards_in_front)
            {
                float obj_min_d = std::numeric_limits<float>::max();
                m_selected_object = m_scene.pickObject(ray, obj_min_d);

                if(m_selected_object)
                {
                    if(obj_min_d < min_d)
                    {
                        m_selected_point_light_id.reset();
                        m_selected_object_blink_elapsed_time = 0.0f;
                    }
                    else
                    {
                        m_selected_object = nullptr;
                    }
                }
            }

            if(m_selected_point_light_id)
            {
                openPointLightEditPanel(*m_selected_point_light_id);
                updatePointLightBillboards();
            }

            if(m_selected_point_light_id || m_selected_object)
            {
                m_mode = Mode::Transform;
            }
        }
    }

    if(Mode::Transform == m_mode)
    {
        switch(m_transform_mode)
        {
        case TransformMode::Move:
            return moveModeHandleEvent(event, input_state);
        case TransformMode::Scale:
            return scaleModeHandleEvent(event, input_state);
        case TransformMode::Rotate:
            return rotModeHandleEvent(event, input_state);
        case TransformMode::None:
            if(event.event == EventType::KeyPressed)
            {
                if(input_state.ctrl())
                {
                    switch(event.key)
                    {
                    case VKeyG:
                        if(m_transform_mode == TransformMode::None)
                        {
                            if(m_selected_object)
                            {
                                m_transform_mode = TransformMode::Move;
                                m_original_pos = m_selected_object->pos();
                            }
                            else if(m_selected_point_light_id)
                            {
                                m_transform_mode = TransformMode::Move;
                                m_original_pos = selectedPointLight().pos;
                            }
                        }
                        break;
                    case VKeyS:
                        if((m_transform_mode == TransformMode::None) && m_selected_object)
                        {
                            m_transform_mode = TransformMode::Scale;
                            m_original_scale = m_selected_object->scale();
                        }
                        break;
                    case VKeyR:
                        if((m_transform_mode == TransformMode::None) && m_selected_object)
                        {
                            m_transform_mode = TransformMode::Rotate;
                            m_original_rot = m_selected_object->rot();
                            m_rot_cursor_pos = input_state.cursor_pos - vec2(m_window.width() / 2.0f, m_window.height() / 2.0f);
                        }
                        break;
                    }
                }

                switch(event.key)
                {
                case VKeyDelete:
                    if(m_selected_object)
                    {
                        m_scene.removeObject(m_selected_object);
                        deselectAll();
                    }
                    else if(m_selected_point_light_id)
                    {
                        m_scene.removeStaticPointLight(*m_selected_point_light_id);
                        deselectAll();
                        updatePointLightBillboards();
                    }
                    break;
                case VKeyEsc:
                    deselectAll();
                    break;
                default:
                    break;
                }
            }
            return;
        default:
            throw std::runtime_error("Invalid TransformMode in Editor: " + std::to_string(static_cast<uint32_t>(m_transform_mode)));
        }
    }

    if(event.event == EventType::KeyPressed)
    {
        if(input_state.ctrl())
        {
            if(event.key == VKeyT)
            {
                if(Mode::Terrain == m_mode)
                {
                    m_mode = Mode::None;
                }
                else
                {
                    m_mode = Mode::Terrain;
                }
            }
            else if(event.key == VKeyA)
            {
                if(m_object_add_panel->isVisible())
                {
                    m_object_add_panel->setVisible(false);

                    if(m_keyboard_focus == m_object_add_panel)
                    {
                        resetKeyboardFocus();
                    }

                    if(m_mouse_focus == m_object_add_panel)
                    {
                        resetMouseFocus();
                    }
                }
                else
                {
                    m_object_add_panel->setVisible(true);
                    setKeyboardFocus(m_object_add_panel);
                }

                return;
            }
            else if(event.key == VKeyP)
            {
                PointLight pl;
                pl.pos = m_scene.camera().pos() + 10.0f * m_scene.camera().forward();
                pl.color = ColorRGBA::White;
                pl.a0 = 1.0f;
                pl.a1 = 1.0f;
                pl.a2 = 1.0f;
                pl.max_d = 100.0f;
                pl.power = 100.0f;
                pl.shadow_map_res = 512;
                m_scene.addStaticPointLight(pl);
                updatePointLightBillboards();
                return;
            }
        }
        else if(Mode::Terrain == m_mode)
        {
            if(event.key == VKeyZ)
            {
                m_scene.terrain().toggleWireframe();
            }
            else if(event.key == VKeyX)
            {
                m_scene.terrain().toggleLod();
            }
        }
        else if(event.key == VKeyF5)
        {
            m_scene.serialize("scene.scn");
        }
    }
}

vec4 Editor::highlightColor() const
{
    return vec4(0.5f, 0.5f, 1.0f, m_selected_object_alpha);
}

void Editor::curTerrainPos(bool& cur_terrain_intersection, vec3& cur_terrain_pos) const
{
    cur_terrain_intersection = (Mode::Terrain == m_mode) && m_cur_terrain_intersection;
    cur_terrain_pos = m_cur_terrain_pos;
}

void Editor::terrainToolRadii(float& inner, float& outer)
{
    inner = m_terrain_tool_radius - m_terrain_tool_width;
    outer = m_terrain_tool_radius;
}

void Editor::moveModeHandleEvent(const Event& event, const InputState& input_state)
{
    Camera& camera = m_scene.camera();

    if(event.event == EventType::MouseMoved)
    {
        vec3 v = 0.01f * (camera.right() * event.cursor_delta.x + camera.up() * -event.cursor_delta.y);

        switch(m_axis_lock)
        {
        case Axis::X:
            v.y = 0.0f;
            v.z = 0.0f;
            break;
        case Axis::Y:
            v.x = 0.0f;
            v.z = 0.0f;
            break;
        case Axis::Z:
            v.x = 0.0f;
            v.y = 0.0f;
            break;
        default:
            break;
        }

        if(m_selected_object)
        {
            m_selected_object->move(v);
        }
        else if(m_selected_point_light_id)
        {
           selectedPointLight().pos += v;
           updateSelectedPointLight();
        }
    }
    else if(event.event == EventType::MousePressed)
    {
        if(event.mouse == LMB)
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.mouse == RMB)
        {
            if(m_selected_object)
            {
                m_selected_object->setPos(m_original_pos);
            }
            else if(m_selected_point_light_id)
            {
                selectedPointLight().pos = m_original_pos;
                updateSelectedPointLight();
            }

            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
    }
    else if(event.event == EventType::KeyPressed)
    {
        if((event.key == VKeyG) && (input_state.shift()))
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyEnter)
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyEsc)
        {
            if(m_selected_object)
            {
                m_selected_object->setPos(m_original_pos);
            }
            else if(m_selected_point_light_id)
            {
                selectedPointLight().pos = m_original_pos;
                updateSelectedPointLight();
            }

            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyX)
        {
            if (m_axis_lock == Axis::X)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::X;
            }
        }
        else if(event.key == VKeyY)
        {
            if (m_axis_lock == Axis::Y)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::Y;
            }
        }
        else if(event.key == VKeyZ)
        {
            if (m_axis_lock == Axis::Z)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::Z;
            }
        }
    }
}

void Editor::scaleModeHandleEvent(const Event& event, const InputState& input_state)
{
    Camera& camera = m_scene.camera();

    if(event.event == EventType::MouseMoved)
    {
        vec3 v = 0.01f * (camera.right() * event.cursor_delta.x + camera.up() * -event.cursor_delta.y);

        switch(m_axis_lock)
        {
        case Axis::None:
            v = vec3(event.cursor_delta.x >= 0.0f ? length(v) : -length(v));
            break;
        case Axis::X:
            v.y = 0.0f;
            v.z = 0.0f;
            break;
        case Axis::Y:
            v.x = 0.0f;
            v.z = 0.0f;
            break;
        case Axis::Z:
            v.x = 0.0f;
            v.y = 0.0f;
            break;
        default:
            throw std::runtime_error("Incorrect Axis!");
        }

        m_selected_object->setScale(m_selected_object->scale() + v);
    }
    else if(event.event == EventType::MousePressed)
    {
        if(event.mouse == LMB)
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.mouse == RMB)
        {
            m_selected_object->setScale(m_original_scale);
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
    }
    else if(event.event == EventType::KeyPressed)
    {
        if((event.key == VKeyS) && (input_state.shift()))
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyEnter)
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyEsc)
        {
            m_selected_object->setScale(m_original_scale);
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyX)
        {
            if (m_axis_lock == Axis::X)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::X;
            }
        }
        else if(event.key == VKeyY)
        {
            if (m_axis_lock == Axis::Y)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::Y;
            }
        }
        else if(event.key == VKeyZ)
        {
            if (m_axis_lock == Axis::Z)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::Z;
            }
        }
    }
}

void Editor::rotModeHandleEvent(const Event& event, const InputState& input_state)
{
    Camera& camera = m_scene.camera();

    if(event.event == EventType::MouseMoved)
    {
        const vec2 curr_cursor_pos = input_state.cursor_pos - vec2(m_window.width() / 2.0f, m_window.height() / 2.0f);

        const auto dot_product = dot(m_rot_cursor_pos, curr_cursor_pos) / (m_rot_cursor_pos.length() * curr_cursor_pos.length());
        const auto cross_product = (m_rot_cursor_pos.x * curr_cursor_pos.y - m_rot_cursor_pos.y * curr_cursor_pos.x) / (m_rot_cursor_pos.length() * curr_cursor_pos.length());
        float a = -std::atan2(cross_product, dot_product);

//        if(a < 0)
//        {
//            a += 2.0f * M_PI;
//        }

        m_rot_cursor_pos = curr_cursor_pos;

        switch(m_axis_lock)
        {
        case Axis::None:
            m_selected_object->rotate(camera.forward(), a);
            break;
        case Axis::X:
            m_selected_object->rotateX(a);
            break;
        case Axis::Y:
            m_selected_object->rotateY(a);
            break;
        case Axis::Z:
            m_selected_object->rotateZ(a);
            break;
        default:
            throw std::runtime_error("Incorrect Axis!");
        }
    }
    else if(event.event == EventType::MousePressed)
    {
        if(event.mouse == LMB)
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.mouse == RMB)
        {
            m_selected_object->setRotation(m_original_rot);
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
    }
    else if(event.event == EventType::KeyPressed)
    {
        if((event.key == VKeyR) && (input_state.shift()))
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyEnter)
        {
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyEsc)
        {
            m_selected_object->setRotation(m_original_rot);
            m_transform_mode = TransformMode::None;
            m_axis_lock = Axis::None;
        }
        else if(event.key == VKeyX)
        {
            if (m_axis_lock == Axis::X)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::X;
            }
        }
        else if(event.key == VKeyY)
        {
            if (m_axis_lock == Axis::Y)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::Y;
            }
        }
        else if(event.key == VKeyZ)
        {
            if (m_axis_lock == Axis::Z)
            {
                m_axis_lock = Axis::None;
            }
            else
            {
                m_axis_lock = Axis::Z;
            }
        }
    }
}

void Editor::deselectAll()
{
    m_selected_object = nullptr;

    m_mode = Mode::None;
    m_transform_mode = TransformMode::None;
    m_axis_lock = Axis::None;

    if(m_selected_point_light_id)
    {
        closePointLightEditPanel();
        m_selected_point_light_id.reset();
        updatePointLightBillboards();
    }
}

PointLight& Editor::selectedPointLight()
{
    return m_scene.staticPointLights()[*m_selected_point_light_id];
}

void Editor::openPointLightEditPanel(PointLightId point_light_id)
{
    m_gui_objects.push_back(std::make_unique<PointLightEditPanel>(m_engine3d, 100.0f, 100.0f, m_scene, point_light_id, m_font));
    m_point_light_edit_panel_id = m_gui_objects.size() - 1;
}

void Editor::closePointLightEditPanel()
{
    m_gui_objects.erase(m_gui_objects.begin() + m_point_light_edit_panel_id);
}

void Editor::updateSelectedPointLight()
{
    m_scene.updateStaticPointLight(*m_selected_point_light_id, selectedPointLight());

    VertexQuad& vq = m_vertex_quad_data[*m_selected_point_light_id];
    vq.T = glm::translate(selectedPointLight().pos) * glm::scale(vec3(m_billboard_size_x, m_billboard_size_y, 1.0f));
    m_engine3d.updateVertexData(m_billboard_vb_alloc.vb, m_billboard_vb_alloc.data_offset + sizeof(VertexQuad) * *m_selected_point_light_id, sizeof(VertexQuad), &m_vertex_quad_data[*m_selected_point_light_id]);
}

void Editor::updatePointLightBillboards()
{
    /*point light billboards*/
    m_vertex_quad_data.resize(m_scene.staticPointLights().size());

    for(size_t i = 0; i < m_scene.staticPointLights().size(); i++)
    {
        VertexQuad& vq = m_vertex_quad_data[i];
        vq.color = m_selected_point_light_id && i == *m_selected_point_light_id ? ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f) : ColorRGBA(0.5f, 0.5f, 0.5f, 1.0f);
        vq.use_texture = 1;
        vq.tex_id = m_engine3d.loadTexture("point_light.png");
        vq.layer_id = 0;
        vq.T = glm::translate(m_scene.staticPointLights()[i].pos) * glm::scale(vec3(m_billboard_size_x, m_billboard_size_y, 1.0f));
    }

    m_engine3d.updateVertexData(m_billboard_vb_alloc.vb, m_billboard_vb_alloc.data_offset, sizeof(VertexQuad) * m_vertex_quad_data.size(), m_vertex_quad_data.data());
}

#endif //EDITOR_ENABLE
