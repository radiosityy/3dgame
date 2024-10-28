#ifndef GAME_H
#define GAME_H

#include "window.h"
#include "renderer.h"
#include "scene.h"
#include "gameplay.h"
#include "console.h"
#include "timer.h"
#include "font.h"
#if EDITOR_ENABLE
#include "editor/editor.h"
#endif

class Game
{
public:
    Game();

    Game(const Game&) = delete;
    Game(const Game&&) = delete;
    Game& operator=(const Game&) = delete;
    Game& operator=(const Game&&) = delete;

    void run();

    void onKeyPressed(Key, const InputState&);
    void onKeyReleased(Key, const InputState&);
    void onMousePressed(MouseButton mb, const InputState&);
    void onMouseReleased(MouseButton mb, const InputState&);
    void onMouseMoved(vec2, const InputState&);
    void onMouseScrolledUp(const InputState&);
    void onMouseScrolledDown(const InputState&);
#if 0
    void onControllerEvent(const ControllerEvent& event, const ControllerState& state);
#endif
    void onWindowResize(uint32_t width, uint32_t height) noexcept;
    void onWindowDestroy() noexcept;

private:
    void setDefaultIni();
    void writeIni();
    void parseIni();
    void updateFpsLabel();

    void stop() noexcept;

    void processConsoleCmd(const std::string&);

    struct InitParams
    {
        uint32_t res_x;
        uint32_t res_y;
        bool vsync;
    } m_init_params;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Gameplay> m_gameplay;
    Timer m_timer;

    bool m_stop = false;
    bool m_invalid_window = false;

    std::vector<Font> m_fonts;

    /*--- gui ---*/
    vec2 m_gui_scale = vec2(1.0f, 1.0f);
    bool m_console_active = false;

    std::unique_ptr<Console> m_console;
    std::unique_ptr<Label> m_fps_label;

#if EDITOR_ENABLE
    bool m_edit_mode = true;
    std::unique_ptr<Editor> m_editor;
#endif

    static inline const auto ini_filename = "game.ini";

    /*--- params ---*/
    constexpr static vec2 reference_resolution = vec2(1920, 1080);
    constexpr static float dt = 1.0f / 240.0f;
    constexpr static float max_frametime = 0.05f;
};

#endif // GAME_H
