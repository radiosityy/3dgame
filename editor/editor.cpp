#if EDITOR_ENABLE
#include "editor.h"
#include "game_utils.h"

Editor::Editor(Window& window, Scene& scene, Renderer& renderer, const Font& font)
    : m_scene(scene)
    , m_window(window)
    , m_renderer(renderer)
    , m_font(font)
{
    m_billboard_vb_alloc = m_renderer.requestVertexBufferAllocation<VertexBillboard>(MAX_POINT_LIGHT_COUNT);
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

        if(m_cur_terrain_intersection)
        {
            if(input_state.mouse & LMB)
            {
                m_scene.terrain().toolEdit(m_renderer, m_cur_terrain_pos, m_terrain_tool_radius, 1.0f * dt);
            }
            else if(input_state.mouse & RMB)
            {
                m_scene.terrain().toolEdit(m_renderer, m_cur_terrain_pos, m_terrain_tool_radius, -1.0f * dt);
            }
        }
    }
    else if(m_selected_object)
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

    updateChildren(m_renderer);

    if(m_object_add_panel && m_object_add_panel->requestedClose())
    {
        closeObjectAddPanel();
    }
}

void Editor::draw(RenderData& render_data)
{
    render_data.cur_terrain_intersection = (Mode::Terrain == m_mode) && m_cur_terrain_intersection;
    render_data.cur_terrain_pos = m_cur_terrain_pos;
    render_data.editor_terrain_tool_inner_radius = m_terrain_tool_radius - m_terrain_tool_width;
    render_data.editor_terrain_tool_outer_radius = m_terrain_tool_radius;
    render_data.editor_highlight_color = vec4(0.5f, 0.5f, 1.0f, m_selected_object_alpha);

    /*selected object highlight*/
    if(m_selected_object)
    {
        m_selected_object->drawHighlight(m_renderer);
    }

    /*point light billboards*/
    m_renderer.draw(RenderMode::Billboard, m_billboard_vb_alloc.vb, m_billboard_vb_alloc.vertex_offset, m_vertex_billboard_data.size(), 0, {});

    drawChildren(m_renderer);
}

void Editor::onWindowResize(uint32_t width, uint32_t height)
{

}

void Editor::deselectAll()
{
    m_selected_object = nullptr;

    m_mode = Mode::None;
    m_axis_lock = Axis::None;

    if(m_selected_point_light_id)
    {
        closePointLightEditPanel();
        m_selected_point_light_id.reset();
        updatePointLightBillboards();
    }
}

void Editor::toggleAxis(Axis axis)
{
    if(m_axis_lock == axis)
    {
        m_axis_lock = Axis::None;
    }
    else
    {
        m_axis_lock = axis;
    }
}

void Editor::confirmTransform()
{
    m_mode = Mode::None;
    m_axis_lock = Axis::None;
}

void Editor::cancelTransform()
{
    switch(m_mode)
    {
    case Mode::Move:
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
        break;
    }
    case Mode::Scale:
    {
        if(m_selected_object)
        {
            m_selected_object->setScale(m_original_scale);
        }
        break;
    }
    case Mode::Rotate:
    {
        if(m_selected_object)
        {
            m_selected_object->setRotation(m_original_rot);
        }
        break;
    }
    default:
        error("cancelTrasform() called when a non-trasform mode is set.");
    }

    m_mode = Mode::None;
    m_axis_lock = Axis::None;
}

PointLight& Editor::selectedPointLight()
{
    return m_scene.staticPointLights()[*m_selected_point_light_id];
}

void Editor::openObjectAddPanel()
{
    m_object_add_panel = addChild(std::make_unique<ObjectAddPanel>(m_renderer, 400.0f, 400.0f, m_scene, m_font));
    setKeyboardFocus(m_object_add_panel);
}

void Editor::closeObjectAddPanel()
{
    removeChild(m_object_add_panel);
    m_object_add_panel = nullptr;
}

void Editor::openPointLightEditPanel(PointLightId point_light_id)
{
    m_point_light_edit_panel = addChild(std::make_unique<PointLightEditPanel>(m_renderer, 100.0f, 100.0f, m_scene, point_light_id, m_font));
    setKeyboardFocus(m_point_light_edit_panel);
}

void Editor::closePointLightEditPanel()
{
    removeChild(m_point_light_edit_panel);
    m_point_light_edit_panel = nullptr;
}

void Editor::updateSelectedPointLight()
{
    m_scene.updateStaticPointLight(*m_selected_point_light_id, selectedPointLight());

    auto& v = m_vertex_billboard_data[*m_selected_point_light_id];

    v.center_pos = selectedPointLight().pos;
    m_renderer.updateVertexData(m_billboard_vb_alloc.vb, m_billboard_vb_alloc.data_offset + sizeof(VertexBillboard) * *m_selected_point_light_id, sizeof(VertexBillboard), &m_vertex_billboard_data[*m_selected_point_light_id]);
}

void Editor::updatePointLightBillboards()
{
    /*point light billboards*/
    m_vertex_billboard_data.resize(m_scene.staticPointLights().size());

    if(!m_scene.staticPointLights().empty())
    {
        for(size_t i = 0; i < m_scene.staticPointLights().size(); i++)
        {
            auto& v = m_vertex_billboard_data[i];
            v.color = m_selected_point_light_id && i == *m_selected_point_light_id ? ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f) : ColorRGBA(0.5f, 0.5f, 0.5f, 1.0f);
            v.tex_id = m_renderer.loadTexture("point_light.png");
            v.layer_id = 0;
            v.center_pos = m_scene.staticPointLights()[i].pos;
            v.size = vec2(m_billboard_size_x, m_billboard_size_y);
        }

        m_renderer.updateVertexData(m_billboard_vb_alloc.vb, m_billboard_vb_alloc.data_offset, sizeof(VertexBillboard) * m_vertex_billboard_data.size(), m_vertex_billboard_data.data());
    }
}

bool Editor::onKeyPressedIntercept(Key key, const InputState& input_state)
{
    if(input_state.ctrl())
    {
        if(key == VKeyA)
        {
            if(m_object_add_panel)
            {
                closeObjectAddPanel();
            }
            else
            {
                openObjectAddPanel();
            }
            return true;
        }
    }

    return false;
}

void Editor::onKeyPressedImpl(Key key, const InputState& input_state)
{
    if(key == VKeyF5)
    {
        m_scene.serialize("scene.scn");
        return;
    }

    switch(m_mode)
    {
    case Mode::None:
    {
        if(input_state.ctrl())
        {
            switch(key)
            {
            case VKeyG:
            {
                if(m_selected_object)
                {
                    m_mode = Mode::Move;
                    m_original_pos = m_selected_object->pos();
                }
                else if(m_selected_point_light_id)
                {
                    m_mode = Mode::Move;
                    m_original_pos = selectedPointLight().pos;
                }
                return;
            }
            case VKeyS:
            {
                if(m_selected_object)
                {
                    m_mode = Mode::Scale;
                    m_original_scale = m_selected_object->scale();
                }
                return;
            }
            case VKeyR:
            {
                if(m_selected_object)
                {
                    m_mode = Mode::Rotate;
                    m_original_rot = m_selected_object->rot();
                    m_rot_cursor_pos = input_state.cursor_pos - vec2(m_window.width() / 2.0f, m_window.height() / 2.0f);
                }
                return;
            }
            case VKeyT:
            {
                if(Mode::Terrain == m_mode)
                {
                    m_mode = Mode::None;
                }
                else
                {
                    m_mode = Mode::Terrain;
                }
                return;
            }
            case VKeyP:
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
        }

        switch(key)
        {
        case VKeyDelete:
        {
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
            return;
        }
        case VKeyEsc:
        {
            deselectAll();
            return;
        }
        }
        return;
    }
    case Mode::Move:
    case Mode::Scale:
    case Mode::Rotate:
    {
        switch(key)
        {
        case VKeyEnter:
        {
            confirmTransform();
            return;
        }
        case VKeyEsc:
        {
            cancelTransform();
            return;
        }
        case VKeyX:
        {
            toggleAxis(Axis::X);
            return;
        }
        case VKeyY:
        {
            toggleAxis(Axis::Y);
            return;
        }
        case VKeyZ:
        {
            toggleAxis(Axis::Z);
            return;
        }
        }
    }
    case Mode::Terrain:
    {
        switch(key)
        {
        case VKeyZ:
            m_scene.terrain().toggleWireframe();
            return;
        case VKeyX:
            m_scene.terrain().toggleLod();
            return;
        }
    }
    }
}

void Editor::onMousePressedImpl(MouseButton mb, const InputState& input_state)
{
    switch(m_mode)
    {
    case Mode::None:
    {
        if(LMB == mb)
        {
            deselectAll();

            const float aspect_ratio = m_scene.camera().aspectRatio();
            const float dx = 2.0f * aspect_ratio / m_window.width();
            const float dy = 2.0f / m_window.height();

            const vec3 ray_dirV = vec3(-aspect_ratio + dx * (0.5f + input_state.cursor_pos.x),
                                       1.0f - dy * (0.5f + input_state.cursor_pos.y),
                                       m_scene.camera().imagePlaneDistance());

            Ray ray(m_scene.camera().pos(), m_scene.camera().invV() * vec4(ray_dirV, 0.0f));
            float min_d = std::numeric_limits<float>::max();

            if(m_render_point_light_billboards)
            {
                /*for every point light we test ray intersection with the 2 triangles
                that compose the point light billboard in editor*/
                const vec3 up = m_scene.camera().up() * m_billboard_size_y;

                for(uint32_t i = 0; i < m_scene.staticPointLights().size(); i++)
                {
                    const auto& point_light = m_scene.staticPointLights()[i];

                    const vec3 right = normalize(cross(up, m_scene.camera().pos() - point_light.pos)) * m_billboard_size_x;

                    float d;

                    const vec3 p0 = point_light.pos - 0.5f * right + 0.5f * up;
                    const vec3 p1 = point_light.pos + 0.5f * right + 0.5f * up;
                    const vec3 p2 = point_light.pos - 0.5f * right - 0.5f * up;

                    if(intersect(ray, p0, p1, p2, d))
                    {
                        if(d < min_d)
                        {
                            min_d = d;
                            m_selected_point_light_id = i;
                            continue;
                        }
                    }

                    const vec3 p3 = point_light.pos + 0.5f * right - 0.5f * up;

                    if(intersect(ray, p2, p1, p3, d))
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
        }
        return;
    }
    case Mode::Move:
    case Mode::Scale:
    case Mode::Rotate:
    {
        if(LMB == mb)
        {
            confirmTransform();
        }
        else if(RMB == mb)
        {
            cancelTransform();
        }
        return;
    }
    }
}

void Editor::onMouseMovedImpl(vec2 cursor_delta, const InputState& input_state)
{
    if(!input_state.mouse)
    {
        switch(m_mode)
        {
        case Mode::Move:
        {
            Camera& camera = m_scene.camera();
            vec3 v = 0.01f * (camera.right() * cursor_delta.x + camera.up() * -cursor_delta.y);

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
            return;
        }
        case Mode::Scale:
        {
            Camera& camera = m_scene.camera();
            vec3 v = 0.01f * (camera.right() * cursor_delta.x + camera.up() * -cursor_delta.y);

            switch(m_axis_lock)
            {
            case Axis::None:
                v = vec3(cursor_delta.x >= 0.0f ? length(v) : -length(v));
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
                error("Incorrect Axis!");
            }

            m_selected_object->setScale(m_selected_object->scale() + v);
            return;
        }
        case Mode::Rotate:
        {
            Camera& camera = m_scene.camera();
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
                error("Incorrect Axis!");
            }
            return;
        }
        }
    }
    else if(input_state.mouse & MMB)
    {
        m_scene.camera().rotate(cursor_delta.x * 0.005f);
        m_scene.camera().pitch(cursor_delta.y * 0.005f);
    }
}

void Editor::onMouseScrolledUpImpl(const InputState& input_state)
{
    if(Mode::Terrain == m_mode)
    {
        m_terrain_tool_radius += 1.0f;
    }
}

void Editor::onMouseScrolledDownImpl(const InputState& input_state)
{
    if(Mode::Terrain == m_mode)
    {
        m_terrain_tool_radius -= 1.0f;
        if(m_terrain_tool_radius < m_min_terrain_tool_radius)
        {
            m_terrain_tool_radius = m_min_terrain_tool_radius;
        }
    }
}

#endif //EDITOR_ENABLE
