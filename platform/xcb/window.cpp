#include "window.h"
#include "game_engine.h"
#include "game_utils.h"

#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <print>

xcb_connection_t* Window::xcbConnection() const noexcept
{
    return m_connection;
}

xcb_window_t* Window::xcbWindow() const noexcept
{
    return m_window;
}

const InputState& Window::inputState() const
{
    return m_input_state;
}

bool Window::hasFocus() const noexcept
{
    return m_has_focus;
}

bool Window::isCursorVisible() const noexcept
{
    return m_cursor_visible;
}

void Window::toggleCursorVisible()
{
    showCursor(!m_cursor_visible);
}

void Window::toggleCursorLock()
{
    if(m_cursor_locked)
    {
        unlockCursor();
    }
    else
    {
        lockCursor();
    }
}

void Window::handleEvents()
{
    std::vector<xcb_generic_event_t*> generic_events;

    bool ignore_remaining_mouse_move_events = false;

    while(true)
    {
        xcb_generic_event_t* generic_event = xcb_poll_for_event(m_connection);

        if(generic_event)
        {
            if(XCB_DESTROY_NOTIFY == generic_event->response_type)
            {
                m_game_engine.onWindowDestroyEvent();
                return;
            }
            generic_events.push_back(generic_event);
        }
        else
        {
            break;
        }
    }

    for(size_t i = 0; i < generic_events.size(); i++)
    {
        auto generic_event = generic_events[i];

        switch(generic_event->response_type)
        {
        case XCB_BUTTON_PRESS:
        {
            if(!m_has_focus)
            {
                break;
            }

            xcb_button_press_event_t xcb_event;
            std::memcpy(&xcb_event, generic_event, sizeof(xcb_event));

            Event event;

            switch(xcb_event.detail)
            {
            case 1: //LMB
                event.event = EventType::MousePressed;
                event.mouse = LMB;
                m_input_state.mouse |= event.mouse;
                break;
            case 2: //MMB
                event.event = EventType::MousePressed;
                event.mouse = MMB;
                m_input_state.mouse |= event.mouse;
                break;
            case 3: //RMB
                event.event = EventType::MousePressed;
                event.mouse = RMB;
                m_input_state.mouse |= event.mouse;
                break;
            case 4: //mouse wheel up
                event.event = EventType::MouseScrolledUp;
                break;
            case 5: //mouse wheel down
                event.event = EventType::MouseScrolledDown;
            default:
                break;
            }

            m_input_state.caps_lock = xcb_event.state & 0x2;

            m_game_engine.onInputEvent(event, m_input_state);

            break;
        }
        case XCB_BUTTON_RELEASE:
        {
            if(!m_has_focus)
            {
                break;
            }

            xcb_button_release_event_t xcb_event;
            std::memcpy(&xcb_event, generic_event, sizeof(xcb_event));

            Event event;
            event.event = EventType::MouseReleased;

            switch(xcb_event.detail)
            {
            case 1: //LMB
                event.mouse = LMB;
                break;
            case 2: //MMB
                event.mouse = MMB;
                break;
            case 3: //RMB
                event.mouse = RMB;
                break;
            default:
                break;
            }

            m_input_state.mouse &= ~event.mouse;
            m_input_state.caps_lock = xcb_event.state & 0x2;

            m_game_engine.onInputEvent(event, m_input_state);

            break;
        }
        case XCB_ENTER_NOTIFY:
        {
            xcb_enter_notify_event_t xcb_event;
            std::memcpy(&xcb_event, generic_event, sizeof(xcb_event));

            m_input_state.cursor_pos = vec2(xcb_event.event_x, xcb_event.event_y);

            break;
        }
        case XCB_MOTION_NOTIFY:
        {
            if(!m_has_focus)
            {
                break;
            }

            if(ignore_remaining_mouse_move_events)
            {
                break;
            }

            if(m_ignore_next_mouse_move_event)
            {
                m_ignore_next_mouse_move_event = false;
                break;
            }

            xcb_motion_notify_event_t xcb_event;
            std::memcpy(&xcb_event, generic_event, sizeof(xcb_event));

            Event event;

            uint32_t x = xcb_event.event_x;
            uint32_t y = xcb_event.event_y;

            if(m_ignore_next_mouse_move_event)
            {
                m_ignore_next_mouse_move_event = false;
                break;
            }

            event.cursor_delta = vec2(x, y) - m_input_state.cursor_pos;

            /*wrap cursor if locked*/
            if(m_cursor_locked)
            {
                bool wrap = false;

                if(x == 0)
                {
                    x = m_width - 2;
                    wrap = true;
                }
                else if(x == m_width - 1)
                {
                    x = 1;
                    wrap = true;
                }

                if(y == 0)
                {
                    y = m_height - 2;
                    wrap = true;
                }
                else if(y == m_height - 1)
                {
                    y = 1;
                    wrap = true;
                }

                if(wrap)
                {
                    m_ignore_next_mouse_move_event = true;
                    ignore_remaining_mouse_move_events = true;
                    xcb_warp_pointer(m_connection, XCB_NONE, m_window, 0, 0, 0, 0, x, y);
                    xcb_flush(m_connection);
                }
            }

            m_input_state.cursor_pos = vec2(x, y);

            /* check state for buttons pressed */
            if(xcb_event.state & 0x700)
            {
                event.event = EventType::MouseDragged;
            }
            else
            {
                event.event = EventType::MouseMoved;
            }

            m_input_state.caps_lock = xcb_event.state & 0x2;

            m_game_engine.onInputEvent(event, m_input_state);

            break;
        }
        case XCB_KEY_PRESS:
        {
            if(!m_has_focus)
            {
                break;
            }

            xcb_key_press_event_t xcb_event;
            std::memcpy(&xcb_event, generic_event, sizeof(xcb_event));

            Event event;
            event.event = EventType::KeyPressed;
            event.key = xcb_event.detail;

            m_input_state.keyboard[event.key] = true;
            m_input_state.caps_lock = xcb_event.state & 0x2;

            m_game_engine.onInputEvent(event, m_input_state);

            break;
        }
        case XCB_KEY_RELEASE:
        {
            if(!m_has_focus)
            {
                break;
            }

            xcb_key_release_event_t xcb_event;
            std::memcpy(&xcb_event, generic_event, sizeof(xcb_event));

            //if the next event is a key press of the same combination,
            //then it's autorepeat and the key wasn't actually released and we ignore this event
            //(this is to not receive alternating press/release events when holding a key)
            if((i < generic_events.size() - 1) && (generic_events[i+1]->response_type == XCB_KEY_PRESS))
            {
                xcb_key_press_event_t next_xcb_event;
                std::memcpy(&next_xcb_event, generic_events[i+1], sizeof(next_xcb_event));

                if(next_xcb_event.detail == xcb_event.detail)
                {
                    break;
                }
            }

            Event event;
            event.event = EventType::KeyReleased;
            event.key = xcb_event.detail;

            m_input_state.keyboard[event.key] = false;
            m_input_state.caps_lock = xcb_event.state & 0x2;

            m_game_engine.onInputEvent(event, m_input_state);

            break;
        }
        case XCB_FOCUS_IN:
        {
            m_has_focus = true;

            if(m_cursor_locked_backup)
            {
                lockCursor();
            }

            updateCursorPos();

            if(!m_cursor_visible_backup)
            {
                showCursor(false);
            }

            break;
        }
        case XCB_FOCUS_OUT:
        {
            m_has_focus = false;

            m_cursor_visible_backup = m_cursor_visible;
            m_cursor_locked_backup = m_cursor_locked;

            if(m_cursor_locked)
            {
                unlockCursor();
            }

            if(!m_cursor_visible)
            {
                showCursor(true);
            }

            break;
        }
        case XCB_CONFIGURE_NOTIFY:
        {
            xcb_configure_notify_event_t event;
            std::memcpy(&event, generic_event, sizeof(event));

            const uint32_t old_width = m_width;
            const uint32_t old_height = m_height;

            m_x = event.x;
            m_y = event.y;
            m_width = event.width;
            m_height = event.height;

            updateCursorPos();

            if((m_width != old_width) || (m_height != old_height))
            {
                const float scale_x = static_cast<float>(m_width) / static_cast<float>(old_width);
                const float scale_y = static_cast<float>(m_height) / static_cast<float>(old_height);

                m_game_engine.onWindowResizeEvent(m_width, m_height, scale_x, scale_y);
            }

            break;
        }
        }

        /* free the event after handling */
        free(generic_event);
    }
}

Window::Window(GameEngine& game_engine, std::string_view app_name, uint16_t width, uint16_t height)
    : m_x(0)
    , m_y(0)
    , m_width(width)
    , m_height(height)
    , m_game_engine(game_engine)
{
    /* Open the connection to the X server */
    xcb_connection_t* connection = xcb_connect(NULL, NULL);
    if(auto err = xcb_connection_has_error(connection); err != 0)
    {
        xcb_disconnect(connection);
        error("Error with X server connection.");
    }

    /* Get the first screen */
    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    /* Create the window */
    uint32_t window_id = xcb_generate_id(connection);
    uint32_t event_mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION;
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, window_id, screen->root, m_x, m_y, m_width, m_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, XCB_CW_EVENT_MASK, &event_mask);

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window_id, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8, app_name.size(), app_name.data());
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window_id, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, app_name.size(), app_name.data());
    xcb_flush(connection);

    m_connection = connection;
    m_window = window_id;
}

Window::~Window()
{
    xcb_disconnect(m_connection);
}

void Window::updateCursorPos()
{
    auto pointer_cookie = xcb_query_pointer(m_connection, m_window);
    xcb_generic_error_t* error = nullptr;
    auto reply = xcb_query_pointer_reply(m_connection, pointer_cookie, &error);
    m_input_state.cursor_pos.x = reply->win_x;
    m_input_state.cursor_pos.y = reply->win_y;
}

void Window::lockCursor()
{
    while(true)
    {
        const auto cookie = xcb_grab_pointer(m_connection, 1, m_window, XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, m_window, XCB_NONE, XCB_CURRENT_TIME);

        //TODO: ugly workaround
        xcb_generic_error_t* err = nullptr;
        const auto reply = xcb_grab_pointer_reply(m_connection, cookie, &err);
        if(reply->status == XCB_GRAB_STATUS_SUCCESS)
        {
            break;
        }
    }

    xcb_flush(m_connection);
    m_cursor_locked = true;
}

void Window::unlockCursor()
{
    xcb_ungrab_pointer(m_connection, XCB_CURRENT_TIME);
    xcb_flush(m_connection);
    m_cursor_locked = false;
}

void Window::show()
{
    const auto cookie = xcb_map_window_checked(m_connection, m_window);

    const auto check = xcb_request_check(m_connection, cookie);
    if(NULL != check)
    {
        error(std::format("Failed to display window. Error: {}", check->error_code));
    }
}

void Window::showCursor(bool show)
{
    if(show && !m_cursor_visible)
    {
        static unsigned char curs_windows_bits[] = {
            0xfe, 0x07, 0xfc, 0x07, 0xfa, 0x07, 0xf6, 0x07, 0xee, 0x07, 0xde, 0x07,
            0xbe, 0x07, 0x7e, 0x07, 0xfe, 0x06, 0xfe, 0x05, 0x3e, 0x00, 0xb6, 0x07,
            0x6a, 0x07, 0x6c, 0x07, 0xde, 0x06, 0xdf, 0x06, 0xbf, 0x05, 0xbf, 0x05,
            0x7f, 0x06};

        static unsigned char mask_windows_bits[] = {
            0x01, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x1f, 0x00, 0x3f, 0x00,
            0x7f, 0x00, 0xff, 0x00, 0xff, 0x01, 0xff, 0x03, 0xff, 0x07, 0x7f, 0x00,
            0xf7, 0x00, 0xf3, 0x00, 0xe1, 0x01, 0xe0, 0x01, 0xc0, 0x03, 0xc0, 0x03,
            0x80, 0x01};

        xcb_cursor_t cursor = xcb_generate_id(m_connection);

        auto bitmap = xcb_create_pixmap_from_bitmap_data(m_connection, m_window, curs_windows_bits, 11, 19, 1, 0, 0, nullptr);
        auto mask = xcb_create_pixmap_from_bitmap_data(m_connection, m_window, mask_windows_bits, 11, 19, 1, 0, 0, nullptr);
        xcb_create_cursor(m_connection, cursor, bitmap, mask, 65535, 65535, 65535, 0, 0, 0, 0, 0);

        xcb_free_pixmap(m_connection, bitmap);
        xcb_free_pixmap(m_connection, mask);

        xcb_change_window_attributes(m_connection, m_window, XCB_CW_CURSOR, &cursor);
        xcb_flush(m_connection);

        xcb_free_cursor(m_connection, cursor);

        m_cursor_visible = true;
    }
    else if(!show && m_cursor_visible)
    {
        std::array<uint8_t, 8> curs_bits = {0, 0, 0, 0, 0, 0, 0, 0};

        xcb_cursor_t cursor = xcb_generate_id(m_connection);

        auto bitmap = xcb_create_pixmap_from_bitmap_data(m_connection, m_window, curs_bits.data(), 8, 8, 1, 0, 0, nullptr);
        xcb_create_cursor(m_connection, cursor, bitmap, bitmap, 0, 0, 0, 0, 0, 0, 0, 0);

        xcb_free_pixmap(m_connection, bitmap);

        xcb_change_window_attributes(m_connection, m_window, XCB_CW_CURSOR, &cursor);
        xcb_flush(m_connection);

        xcb_free_cursor(m_connection, cursor);

        m_cursor_visible = false;
    }
}

void Window::setSize(uint32_t width, uint32_t height)
{
    std::array<uint32_t, 4> values = {0, 0, width, height};

    uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    xcb_configure_window(m_connection, m_window, mask, values.data());
    xcb_flush(m_connection);
}

int32_t Window::x() const noexcept
{
    return m_x;
}

int32_t Window::y() const noexcept
{
    return m_y;
}

uint32_t Window::width() const noexcept
{
    return m_width;
}

uint32_t Window::height() const noexcept
{
    return m_height;
}
