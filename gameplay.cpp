#include "gameplay.h"

Gameplay::Gameplay(Renderer& renderer, Scene& scene, float aspect_ratio)
    : m_camera(aspect_ratio, 1.0f, 2000.0f, degToRad(90.0f))
    , m_renderer(renderer)
    , m_scene(scene)
{
    m_camera.setPos(vec3(0.0f, 66.0f, -70.0f));
    m_camera.pitch(glm::radians(45.0f));

    /*add player*/
    m_player = std::make_unique<Player>(renderer, "assets/meshes/man.3d", RenderMode::Default, vec3(0.0f, 5.0f, -10.0f));
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
}

const Camera& Gameplay::camera() const noexcept
{
    return m_camera;
}

void Gameplay::onWindowResize(float aspect_ratio) noexcept
{
    m_camera.setAspectRatio(aspect_ratio);
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

