#ifndef WINDOW_H
#define WINDOW_H

#include "platform.h"
#include <vector>
#include <memory>
#include "event.h"

class GameEngine;

class Window
{
public:
    Window(GameEngine& game_engine, std::string_view app_name, uint16_t width = 1024, uint16_t height = 720);
    ~Window();

    Window(const Window&) = delete;
    Window(const Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(const Window&&) = delete;

#ifdef PLATFORM_WIN32
    LRESULT EventHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

    void show();

    void showCursor(bool);
    void toggleCursorVisible();

    void lockCursor();
    void unlockCursor();
    void toggleCursorLock();

    void manageEvents();

    int32_t x() const noexcept;
    int32_t y() const noexcept;
    uint32_t width() const noexcept;
    uint32_t height() const noexcept;

    const WindowParameters& getParams() const noexcept;
    const InputState& inputState()const;
    bool hasFocus() const noexcept;
    bool isCursorVisible() const noexcept;

    void setSize(uint32_t width, uint32_t height);

private:
    void updateCursorPos();

    int32_t m_x;
    int32_t m_y;
    uint32_t m_width;
    uint32_t m_height;

    GameEngine& m_game_engine;
    WindowParameters m_params;

    InputState m_input_state;
    ControllerState m_controller_state;

    bool m_has_focus = false;
    bool m_cursor_visible = true;
    bool m_cursor_locked = false;
    bool m_cursor_visible_backup = m_cursor_visible;
    bool m_cursor_locked_backup = m_cursor_locked;

    bool m_ignore_next_mouse_move_event = false;

#ifdef PLATFORM_WIN32
    const std::string m_class_name;

    void trackCursor();
    bool m_cursor_left = false;

    void handleControllerEvents();
    DWORD m_prev_xinput_packet_number = 0;
#endif
};

#endif //WINDOW_H
