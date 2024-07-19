#ifndef WINDOW_XCB_H
#define WINDOW_XCB_H

#include "platform.h"
#include <vector>
#include <memory>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
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

    void show();

    void showCursor(bool);
    void toggleCursorVisible();

    void lockCursor();
    void unlockCursor();
    void toggleCursorLock();

    void handleEvents();

    int32_t x() const noexcept;
    int32_t y() const noexcept;
    uint32_t width() const noexcept;
    uint32_t height() const noexcept;

    xcb_connection_t* xcbConnection() const noexcept;
    xcb_window_t* xcbWindow() const noexcept;
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
    xcb_connection_t* m_connection;
    xcb_window_t m_window;

    InputState m_input_state;
    // ControllerState m_controller_state;

    bool m_has_focus = false;
    bool m_cursor_visible = true;
    bool m_cursor_locked = false;
    bool m_cursor_visible_backup = m_cursor_visible;
    bool m_cursor_locked_backup = m_cursor_locked;

    bool m_ignore_next_mouse_move_event = false;
};

#endif //WINDOW_XCB_H
