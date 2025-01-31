#include "gameplay.h"

Gameplay::Gameplay(Window& window, Renderer& renderer, Scene& scene, Font& font)
    : m_window(window)
    , m_camera(static_cast<float>(window.width()) / static_cast<float>(window.height()), 1.0f, 2000.0f, degToRad(90.0f))
    , m_renderer(renderer)
    , m_scene(scene)
{
    m_camera.setPos(vec3(0.0f, 66.0f, -70.0f));
    m_camera.pitch(glm::radians(45.0f));

    /*add player*/
    m_player = std::make_unique<Player>(renderer, "assets/meshes/man.3d", RenderMode::Default, vec3(0.0f, 0.0f, 0.0f));
#if EDITOR_ENABLE
    m_player->setSerializable(false);
#endif

    /*add sun*/
    m_sun.shadow_map_count = 4;
    m_sun.shadow_map_res_x = 2048;
    m_sun.shadow_map_res_y = 2048;
    renderer.addDirLight(m_sun);

    m_time_of_day = 11.0f * 3600.0f;
    updateSun();

    m_camera.setPos(vec3(0.0f, cam_distance, cam_distance));
    const vec3 cam_forward = glm::rotateX(vec3(0.0f, 0.0f, 1.0f), degToRad(45));
    const vec3 cam_up = glm::rotateX(vec3(0.0f, 1.0f, 0.0f), degToRad(45));
    m_camera.setBasis(cam_forward, cam_up, vec3(1.0f, 0.0f, 0.0f));
}

const Camera& Gameplay::camera() const noexcept
{
    return m_camera;
}

void Gameplay::onWindowResize(uint32_t window_width, uint32_t window_height) noexcept
{
    m_camera.setAspectRatio(static_cast<float>(window_width) / static_cast<float>(window_height));
}

void Gameplay::onKeyPressed(Key key, const InputState&, bool repeated)
{
    if(!repeated)
    {
        switch(key)
        {
        case VKeyW:
        {
            vec3 dir = m_camera.forward();
            dir.y = 0.0f;
            dir = glm::normalize(dir);
            m_player->walk(dir);
            break;
        }
        case VKeyS:
        {
            vec3 dir = m_camera.forward();
            dir.y = 0.0f;
            dir = glm::normalize(dir);
            m_player->walk(-dir);
            break;
        }
        case VKeyA:
        {
            vec3 dir = m_camera.right();
            dir.y = 0.0f;
            dir = glm::normalize(dir);
            m_player->walk(-dir);
            break;
        }
        case VKeyD:
        {
            vec3 dir = m_camera.right();
            dir.y = 0.0f;
            dir = glm::normalize(dir);
            m_player->walk(dir);
            break;
        }
        }
    }
}

void Gameplay::onKeyReleased(Key key, const InputState&)
{
    switch(key)
    {
    case VKeyW:
    {
        vec3 dir = m_camera.forward();
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        m_player->walk(-dir);
        break;
    }
    case VKeyS:
    {
        vec3 dir = m_camera.forward();
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        m_player->walk(dir);
        break;
    }
    case VKeyA:
    {
        vec3 dir = m_camera.right();
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        m_player->walk(dir);
        break;
    }
    case VKeyD:
    {
        vec3 dir = m_camera.right();
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        m_player->walk(-dir);
        break;
    }
    }
}

void Gameplay::onMousePressed(MouseButton mb, const InputState&)
{
    if(MMB == mb)
    {
        m_window.showCursor(false);
        m_window.stopCursor(true);
    }
}

void Gameplay::onMouseReleased(MouseButton mb, const InputState&)
{
    if(MMB == mb)
    {
        m_window.showCursor(true);
        m_window.stopCursor(false);
    }
}

void Gameplay::onMouseMoved(vec2 d, const InputState& input_state)
{
    if(input_state.mmb())
    {
        const float a = 5.0f * d.x / static_cast<float>(m_window.width());
        const vec3 cam_forward = glm::rotateY(m_camera.forward(), a);
        const vec3 cam_up = glm::rotateY(m_camera.up(), a);
        m_camera.setBasis(cam_forward, cam_up, glm::cross(cam_forward, cam_up));
        m_camera.setPos(m_player->pos() - cam_distance * cam_forward);
        m_player->setWalkDir(glm::rotateY(m_player->walkDir(), a));
    }

    const vec2 player_forwardV = 2.0f * input_state.cursor_pos / vec2(m_window.width(), m_window.height()) - vec2(1.0f, 1.0f);
    mat4x4 M = m_camera.invV();
    M[0][1] = 0.0f;
    M[0] = glm::normalize(M[0]);
    M[1][1] = 0.0f;
    M[1] = glm::normalize(M[1]);
    M[2][1] = 0.0f;
    M[2] = glm::normalize(M[2]);
    const vec3 player_forwardW = normalize(vec3(M * vec4(player_forwardV.x, 0.0f, -player_forwardV.y, 0.0f)));
    m_player->setForward(player_forwardW);
}

void Gameplay::update(float dt, const InputState& input_state, bool process_input)
{
    /*advance time of day (wrap around after a whole day has passed)*/
    m_time_of_day += dt * m_time_scale;

    if(m_time_of_day >= m_day_length)
    {
        m_time_of_day -= m_day_length;
    }

    // m_time_of_day = 11.0f * 3600.0f;
    // updateSun();

    m_player->move(dt * m_player->velocity());
    m_camera.setPos(m_player->pos() - cam_distance * m_camera.forward());

    m_scene.update(dt, input_state);
    m_player->update(m_renderer, dt);
}

void Gameplay::draw(RenderData& render_data)
{
    m_scene.draw(m_camera, render_data);
    m_player->draw(m_renderer);
    // if(m_player->cull(view_frustum_planes))
    // {
    //     m_player->draw(m_renderer);
    // }
}

void Gameplay::updateSun()
{
    static constexpr float sun_radius = 0.1f;
    static constexpr float lat = degToRad(52.33f);
    static constexpr float decl = degToRad(23.0f);
    static constexpr float utc_offset = 2.0f;
    static constexpr float lst_offset = (12.0f + utc_offset) * 3600.0f;

    const float t = m_time_of_day / m_day_length;

    const float azimuth = pi_2 - 2.0f * pi * t;
    float inclination = pi_2 - std::asin(std::sin(lat) * std::sin(decl) + std::cos(lat) * std::cos(decl) * std::cos(pi / 12.0f * ((m_time_of_day - lst_offset) / 3600.0f)));

    const vec3 visual_sun_pos = vec3(std::cos(azimuth) * std::sin(inclination),
                                     std::cos(inclination),
                                     std::sin(azimuth) * std::sin(inclination));

    if(visual_sun_pos.y < sun_radius)
    {
        const float alpha = std::acos(std::clamp(-visual_sun_pos.y/sun_radius, -1.0f, 1.0f));
        const float visible_sun_area = (2.0f*alpha - std::sin(2.0f*alpha)) / (2.0f * pi);

        inclination -= (4.0f * sun_radius * std::pow(std::sin(alpha), 3.0f)) / (3.0f * (2.0f*alpha - std::sin(2.0f*alpha)));

        m_sun.dir = -vec3(std::cos(azimuth) * std::sin(inclination),
                          std::cos(inclination),
                          std::sin(azimuth) * std::sin(inclination));

        m_sun.color = vec3(0.5f) * visible_sun_area;
    }
    else
    {
        m_sun.dir = -visual_sun_pos;
        m_sun.color = vec3(0.5f);
    }

    m_renderer.updateDirLight(0, m_sun);
}

