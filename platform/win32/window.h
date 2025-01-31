#ifndef WINDOW_WIN32_H
#define WINDOW_WIN32_H

#include <vector>
#include <memory>
#include "input_state.h"

class Game;

class Window
{
public:
    Window(Game& game, std::string_view app_name, uint16_t width = 1024, uint16_t height = 720);
    ~Window();

    Window(const Window&) = delete;
    Window(const Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(const Window&&) = delete;

    LRESULT EventHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void show();

    void showCursor(bool);
    void toggleShowCursor();

    void lockCursor();
    void unlockCursor();
    void toggleCursorLock();

    void wrapCursor(bool);
    void toggleCursorWrap();

    void stopCursor(bool);

    void handleEvents();

    int32_t x() const noexcept;
    int32_t y() const noexcept;
    uint32_t width() const noexcept;
    uint32_t height() const noexcept;

    HWND hwnd() const noexcept;
    HINSTANCE hinstance() const noexcept;
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

    Game& m_game;
    HWND m_hwnd;
    HINSTANCE m_hinstance;

    InputState m_input_state;
    // ControllerState m_controller_state;

    bool m_has_focus = false;
    bool m_cursor_visible = true;
    bool m_cursor_visible_backup = m_cursor_visible;
    bool m_cursor_locked = false;
    bool m_cursor_locked_backup = m_cursor_locked;
    bool m_cursor_wrap = false;
    bool m_cursor_stopped = false;

    bool m_ignore_next_mouse_move_event = false;

    const std::string m_class_name;

    void trackCursor();
    bool m_cursor_left = false;

    void handleControllerEvents();
    DWORD m_prev_xinput_packet_number = 0;
};

#endif //WINDOW_WIN32_H
