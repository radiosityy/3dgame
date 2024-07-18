#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "window.h"
#include "engine_3d.h"
#include "scene.h"
#include "console.h"
#include "timer.h"
#include "font.h"
#if EDITOR_ENABLE
#include "editor/editor.h"
#endif

class GameEngine
{
public:
    GameEngine();

    GameEngine(const GameEngine&) = delete;
    GameEngine(const GameEngine&&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&&) = delete;

    void run();

    void onKeyPressed(Key, const InputState&);
    void onKeyReleased(Key, const InputState&);
    void onMousePressed(MouseButton mb, const InputState&);
    void onMouseReleased(MouseButton mb, const InputState&);
    void onMouseMoved(vec2, const InputState&);
    void onMouseDragged(vec2, const InputState&);
    void onMouseScrolledUp(const InputState&);
    void onMouseScrolledDown(const InputState&);
    //void onControllerEvent(const ControllerEvent& event, const ControllerState& state);
    void onWindowResizeEvent(uint32_t width, uint32_t height, float scale_x, float scale_y) noexcept;
    void onWindowDestroyEvent() noexcept;

private:
    void setDefaultIni();
    void writeIni();
    void parseIni();
    void setupGui();

    void stop() noexcept;

    void processConsoleCmd(const std::string&);

    struct InitParams
    {
        uint32_t res_x;
        uint32_t res_y;
        bool vsync;
    } m_init_params;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Engine3D> m_engine3d;
    std::unique_ptr<Scene> m_scene;
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

#endif // GAME_ENGINE_H
