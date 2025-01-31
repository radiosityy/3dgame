#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "camera.h"
#include "scene.h"
#include "player.h"

class Gameplay
{
public:
    Gameplay(Window& window, Renderer& renderer, Scene& scene, Font& font);

    const Camera& camera() const noexcept;

    void onWindowResize(uint32_t window_width, uint32_t window_height) noexcept;
    void onKeyPressed(Key, const InputState&, bool);
    void onKeyReleased(Key, const InputState&);
    void onMousePressed(MouseButton mb, const InputState&);
    void onMouseReleased(MouseButton mb, const InputState&);
    void onMouseMoved(vec2, const InputState&);

    void update(float dt, const InputState& input_state, bool process_input);
    void draw(RenderData& render_data);

private:
    Window& m_window;
    Camera m_camera;
    Renderer& m_renderer;
    Scene& m_scene;

    void updateSun();

    /*--- Sun ---*/
    DirLight m_sun;

    static constexpr float m_time_scale = 60.0f * 60.0f; //one minute in game is one sec in real life
    static constexpr float m_day_length = 24.0f * 60.0f * 60.0f;
    float m_time_of_day = 0.0f;

    /*--- Player ---*/
    std::unique_ptr<Player> m_player;
    bool m_player_movement = true;

    constexpr static float cam_distance = 30.0f;
};

#endif //GAMEPLAY_H
