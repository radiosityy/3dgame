#include "window.h"
#include "game_engine.h"

#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>

const WindowParameters& Window::getParams() const noexcept
{
    return m_params;
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

/*------------------------------------------------------*/
//    Win32
/*------------------------------------------------------*/

#ifdef PLATFORM_WIN32

LRESULT CALLBACK WindowEventHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //TODO: can we get better performance with a global variable window pointer instead of this
    //Set/GetWindowLongPtr and WM_CREATE bullcrap?
    if(WM_CREATE == uMsg)
    {
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    return window->EventHandler(hwnd, uMsg, wParam, lParam);
}

void Window::manageEvents()
{
    MSG msg;

    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    handleControllerEvents();
}

static ControllerState controllerStateFromXINPUT_GAMEPAD(const XINPUT_GAMEPAD& xinput)
{
    ControllerState state;

    state.buttons[ControllerDPadUp] = xinput.wButtons & XINPUT_GAMEPAD_DPAD_UP;
    state.buttons[ControllerDPadDown] = xinput.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
    state.buttons[ControllerDPadLeft] = xinput.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
    state.buttons[ControllerDPadRight] = xinput.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
    state.buttons[ControllerStart] = xinput.wButtons & XINPUT_GAMEPAD_START;
    state.buttons[ControllerBack] = xinput.wButtons & XINPUT_GAMEPAD_BACK;
    state.buttons[ControllerLeftThumb] = xinput.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
    state.buttons[ControllerRightThumb] = xinput.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
    state.buttons[ControllerLeftBumper] = xinput.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
    state.buttons[ControllerRightBumper] = xinput.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
    state.buttons[ControllerA] = xinput.wButtons & XINPUT_GAMEPAD_A;
    state.buttons[ControllerB] = xinput.wButtons & XINPUT_GAMEPAD_B;
    state.buttons[ControllerX] = xinput.wButtons & XINPUT_GAMEPAD_X;
    state.buttons[ControllerY] = xinput.wButtons & XINPUT_GAMEPAD_Y;

    state.left_trigger = static_cast<float>(xinput.bLeftTrigger) / 255.0f;
    state.right_trigger = static_cast<float>(xinput.bRightTrigger) / 255.0f;

    /*left stick x*/
    if(xinput.sThumbLX < 0)
    {
        if(xinput.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            state.left_stick.x = 0.0f;
        }
        else
        {
            state.left_stick.x = static_cast<float>(xinput.sThumbLX + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32768.0f  - static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
        }
    }
    else
    {
        if(xinput.sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            state.left_stick.x = 0.0f;
        }
        else
        {
            state.left_stick.x = static_cast<float>(xinput.sThumbLX - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32767.0f  - static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
        }
    }

    /*left stick y*/
    if(xinput.sThumbLY < 0)
    {
        if(xinput.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            state.left_stick.y = 0.0f;
        }
        else
        {
            state.left_stick.y = static_cast<float>(xinput.sThumbLY + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32768.0f  - static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
        }
    }
    else
    {
        if(xinput.sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            state.left_stick.y = 0.0f;
        }
        else
        {
            state.left_stick.y = static_cast<float>(xinput.sThumbLY - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32767.0f  - static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
        }
    }

    /*right stick x*/
    if(xinput.sThumbRX < 0)
    {
        if(xinput.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
        {
            state.right_stick.x = 0.0f;
        }
        else
        {
            state.right_stick.x = static_cast<float>(xinput.sThumbRX + XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) / (32768.0f  - static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
        }
    }
    else
    {
        if(xinput.sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
        {
            state.right_stick.x = 0.0f;
        }
        else
        {
            state.right_stick.x = static_cast<float>(xinput.sThumbRX - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) / (32767.0f  - static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
        }
    }

    /*right stick y*/
    if(xinput.sThumbRY < 0)
    {
        if(xinput.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
        {
            state.right_stick.y = 0.0f;
        }
        else
        {
            state.right_stick.y = static_cast<float>(xinput.sThumbRY + XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) / (32768.0f  - static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
        }
    }
    else
    {
        if(xinput.sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
        {
            state.right_stick.y = 0.0f;
        }
        else
        {
            state.right_stick.y = static_cast<float>(xinput.sThumbRY - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) / (32767.0f  - static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
        }
    }

    return state;
}

void Window::handleControllerEvents()
{
    XINPUT_STATE xinput_state{};
    auto res = XInputGetState(0, &xinput_state);

    if(res == ERROR_SUCCESS)
    {
        if(xinput_state.dwPacketNumber != m_prev_xinput_packet_number)
        {
            const auto state = controllerStateFromXINPUT_GAMEPAD(xinput_state.Gamepad);

            /*buttons*/
            for(uint8_t i = 0; i < ControllerButtonCount; i++)
            {
                if(m_controller_state.buttons[i] != state.buttons[i])
                {
                    ControllerEvent event;
                    event.event = state.buttons[i] ? ControllerEventType::ButtonPressed : ControllerEventType::ButtonReleased;
                    event.button = i;

                    m_controller_state.buttons[i] = state.buttons[i];

                    m_game_engine.onControllerEvent(event, m_controller_state);
                }
            }

            /*left trigger*/
            if(m_controller_state.left_trigger != state.left_trigger)
            {
                ControllerEvent event;
                event.event = ControllerEventType::LeftTriggerChange;
                event.trigger_delta = state.left_trigger - m_controller_state.left_trigger;

                m_controller_state.left_trigger = state.left_trigger;

                m_game_engine.onControllerEvent(event, m_controller_state);
            }

            /*right trigger*/
            if(m_controller_state.right_trigger != state.right_trigger)
            {
                ControllerEvent event;
                event.event = ControllerEventType::RightTriggerChange;
                event.trigger_delta = state.right_trigger - m_controller_state.right_trigger;

                m_controller_state.right_trigger = state.right_trigger;

                m_game_engine.onControllerEvent(event, m_controller_state);
            }

            /*left stick*/
            if(m_controller_state.left_stick != state.left_stick)
            {
                ControllerEvent event;
                event.event = ControllerEventType::LeftStickMoved;
                event.stick_delta = state.left_stick - m_controller_state.left_stick;

                m_controller_state.left_stick = state.left_stick;

                m_game_engine.onControllerEvent(event, m_controller_state);
            }

            /*right stick*/
            if(m_controller_state.right_stick != state.right_stick)
            {
                ControllerEvent event;
                event.event = ControllerEventType::RightStickMoved;
                event.stick_delta = state.right_stick - m_controller_state.right_stick;

                m_controller_state.right_stick = state.right_stick;

                m_game_engine.onControllerEvent(event, m_controller_state);
            }

            m_prev_xinput_packet_number = xinput_state.dwPacketNumber;
        }
    }
    else if(res != ERROR_DEVICE_NOT_CONNECTED)
    {
        throw std::runtime_error("Error in XInputGetState" + std::to_string(res));
    }
}

Window::Window(GameEngine& game_engine, std::string_view app_name, uint16_t width, uint16_t height)
    : m_x(0)
    , m_y(0)
    , m_width(width)
    , m_height(height)
    , m_game_engine(game_engine)
    , m_class_name(app_name)
{
    m_params.hinstance = GetModuleHandle(NULL);
    LPCTSTR window_name = m_class_name.c_str();
    LPCTSTR class_name = m_class_name.c_str();

    WNDCLASSEX wnd_class{};
    wnd_class.cbSize = sizeof(WNDCLASSEX);
    wnd_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wnd_class.lpszClassName = window_name;
    wnd_class.hInstance = m_params.hinstance;
    wnd_class.style = CS_HREDRAW | CS_VREDRAW;
    wnd_class.lpfnWndProc = WindowEventHandler;
    wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wnd_class);

    const int left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    m_params.hwnd = CreateWindow(class_name, window_name, WS_POPUP | WS_CLIPCHILDREN, left, top, width, height, NULL, NULL, m_params.hinstance, this);

    if(!m_params.hwnd)
    {
        //TODO: get error from GetLastError()
//        DWORD error = GetLastError();
        throw std::runtime_error("Failed to create window.");
    }

    /*init raw input*/
    RAWINPUTDEVICE devices[2];

    //Mouse
    devices[0].usUsagePage = 0x01;
    devices[0].usUsage = 0x02;
    devices[0].dwFlags = 0;// RIDEV_NOLEGACY;
    devices[0].hwndTarget = m_params.hwnd;

    //Keyboard
    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x06;
    devices[1].dwFlags = RIDEV_NOLEGACY;
    devices[1].hwndTarget = m_params.hwnd;

    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
}

Window::~Window()
{
    UnregisterClass(m_class_name.c_str(), m_params.hinstance);
}

void Window::updateCursorPos()
{
    POINT p;
    GetCursorPos(&p);
    m_input_state.cursor_pos = vec2(p.x - m_x, p.y - m_y);
}

void Window::trackCursor()
{
    TRACKMOUSEEVENT track_mouse_event{};
    track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
    track_mouse_event.dwFlags = TME_LEAVE;
    track_mouse_event.hwndTrack = m_params.hwnd;
    track_mouse_event.dwHoverTime = HOVER_DEFAULT;

    TrackMouseEvent(&track_mouse_event);
}

void Window::lockCursor()
{
    RECT rect;
    rect.left = m_x;
    rect.top = m_y;
    rect.right = m_x + static_cast<int32_t>(m_width);
    rect.bottom = m_y + static_cast<int32_t>(m_height);

    ClipCursor(&rect);

    m_cursor_locked = true;
}

void Window::unlockCursor()
{
    ClipCursor(NULL);
    m_cursor_locked = false;
}

LRESULT Window::EventHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INPUT:
    {
        if(!m_has_focus)
        {
            return 0;
        }

        UINT raw_size;
        RAWINPUT raw;

        //should return 0, otherwise there's an error
        if(0 != GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &raw_size, sizeof(RAWINPUTHEADER)))
        {
            return -1;
        }
        //returns number of bytes copied to raw_data, if not equal to raw_size then there's an error
        if(raw_size != GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &raw_size, sizeof(RAWINPUTHEADER)))
        {
            return -1;
        }

        Event event;

        //Manage keyboard input
        if(raw.header.dwType == RIM_TYPEKEYBOARD)
        {
            if(VK_SHIFT == raw.data.keyboard.VKey)
            {
                if(raw.data.keyboard.MakeCode == 54)
                {
                    event.key = VKeyRShift;
                }
                else if(raw.data.keyboard.MakeCode == 42)
                {
                    event.key = VKeyLShift;
                }
            }
            else if(VK_CONTROL == raw.data.keyboard.VKey)
            {
                if(raw.data.keyboard.Flags & RI_KEY_E0)
                {
                    event.key = VKeyRCtrl;
                }
                else
                {
                    event.key = VKeyLCtrl;
                }
            }
            else if(VK_MENU == raw.data.keyboard.VKey)
            {
                if(raw.data.keyboard.Flags & RI_KEY_E0)
                {
                    event.key = VKeyRAlt;
                }
                else
                {
                    event.key = VKeyLAlt;
                }
            }
            else
            {
                event.key = raw.data.keyboard.VKey;
            }

            if(raw.data.keyboard.Flags & RI_KEY_BREAK)
            {
                event.event = EventType::KeyReleased;
                m_input_state.keyboard[event.key] = false;
            }
            else
            {
                event.event = EventType::KeyPressed;
                m_input_state.keyboard[event.key] = true;
            }
        }
        //Manage mouse input
        else if(raw.header.dwType == RIM_TYPEMOUSE)
        {
            switch(raw.data.mouse.usButtonFlags)
            {
            case RI_MOUSE_LEFT_BUTTON_DOWN:
            {
                event.mouse = LMB;
                event.event = EventType::MousePressed;
                m_input_state.mouse |= event.mouse;
                break;
            }
            case RI_MOUSE_LEFT_BUTTON_UP:
            {
                event.mouse = LMB;
                event.event = EventType::MouseReleased;
                m_input_state.mouse &= ~event.mouse;
                break;
            }
            case RI_MOUSE_RIGHT_BUTTON_DOWN:
            {
                event.mouse = RMB;
                event.event = EventType::MousePressed;
                m_input_state.mouse |= event.mouse;
                break;
            }
            case RI_MOUSE_RIGHT_BUTTON_UP:
            {
                event.mouse = RMB;
                event.event = EventType::MouseReleased;
                m_input_state.mouse &= ~event.mouse;
                break;
            }
            case RI_MOUSE_MIDDLE_BUTTON_DOWN:
            {
                event.mouse = MMB;
                event.event = EventType::MousePressed;
                m_input_state.mouse |= event.mouse;
                break;
            }
            case RI_MOUSE_MIDDLE_BUTTON_UP:
            {
                event.mouse = MMB;
                event.event = EventType::MouseReleased;
                m_input_state.mouse &= ~event.mouse;
                break;
            }
            case RI_MOUSE_WHEEL:
            {
                const auto delta = static_cast<short>(raw.data.mouse.usButtonData);

                if(delta > 0)
                {
                    event.event = EventType::MouseScrolledUp;
                }
                else if(delta < 0)
                {
                    event.event = EventType::MouseScrolledDown;
                }

                break;
            }
            default:
                //handle mouse movement in WM_MOUSEMOVE event, because WM_INPUT SUCKS
                return 0;
            }
        }

        m_input_state.caps_lock = GetKeyState(VK_CAPITAL) & 1;

        m_game_engine.onInputEvent(event, m_input_state);

        return 0;
    }
    case WM_MOUSELEAVE:
        m_cursor_left = true;
        return 0;
    case WM_MOUSEMOVE:
    {
        if(!m_has_focus)
        {
            return 0;
        }

        if(m_ignore_next_mouse_move_event)
        {
            m_ignore_next_mouse_move_event = false;
            return 0;
        }

        //TODO: microsoft documentation says that x and y can be negative here in case of multi display
        //GET_X_LPARAM() and GET_Y_LPARAM() should be used instead of LOWORD() and HIWORD()
        //this will require that I update other parts of code to expect negative values
        //it might also be the case on linux! should check!
        uint32_t x = LOWORD(lParam);
        uint32_t y = HIWORD(lParam);

        Event event;

        if(m_cursor_left)
        {
            m_cursor_left = false;
            updateCursorPos();
            trackCursor();
            event.cursor_delta = vec2(0.0f, 0.0f);
        }
        else
        {
            event.cursor_delta = vec2(x, y) - m_input_state.cursor_pos;
        }

        event.event = (wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)) ? EventType::MouseDragged : EventType::MouseMoved;

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
                SetCursorPos(static_cast<int>(x) + m_x, static_cast<int>(y) + m_y);
            }
        }

        m_input_state.cursor_pos = vec2(x, y);
        m_input_state.caps_lock = GetKeyState(VK_CAPITAL) & 1;

        m_game_engine.onInputEvent(event, m_input_state);

        return 0;
    }
    case WM_MOVE:
    {
        m_x = LOWORD(lParam);
        m_y = HIWORD(lParam);

        if(m_cursor_locked)
        {
            lockCursor();
        }

        updateCursorPos();

        return 0;
    }
    case WM_SIZE:
    {
        const uint32_t old_width = m_width;
        const uint32_t old_height = m_height;

        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);

        if(m_cursor_locked)
        {
            lockCursor();
        }

        updateCursorPos();

        if((m_width != old_width) || (m_height != old_height))
        {
            const float scale_x = static_cast<float>(m_width) / static_cast<float>(old_width);
            const float scale_y = static_cast<float>(m_height) / static_cast<float>(old_height);

            m_game_engine.onWindowResizeEvent(m_width, m_height, scale_x, scale_y);
        }

        return 0;
    }
    case WM_SETFOCUS:
    {
        m_has_focus = true;

        if(!m_cursor_visible_backup)
        {
            showCursor(false);
        }

        if(m_cursor_locked_backup)
        {
            lockCursor();
        }

        updateCursorPos();
        m_input_state.caps_lock = GetKeyState(VK_CAPITAL) & 1;

        return 0;
    }
    case WM_KILLFOCUS:
    {
        m_has_focus = false;

        m_cursor_visible_backup = m_cursor_visible;
        m_cursor_locked_backup = m_cursor_locked;

        if(!m_cursor_visible)
        {
            showCursor(true);
        }

        if(m_cursor_locked)
        {
            unlockCursor();
        }

        return 0;
    }
    case WM_DESTROY:
    {
        //TODO: call onWindowDestroyEvent
        return 0;
    }
    default:
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Window::show()
{
    updateCursorPos();
    ShowWindow(m_params.hwnd, SW_SHOW);
    trackCursor();
}

void Window::showCursor(bool show)
{
    ShowCursor(show);
    m_cursor_visible = show;
}

void Window::setSize(uint32_t width, uint32_t height)
{
    const int x = (GetSystemMetrics(SM_CXSCREEN) - static_cast<int>(width)) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - static_cast<int>(height)) / 2;

    SetWindowPos(m_params.hwnd, HWND_TOP, x, y, static_cast<int>(width), static_cast<int>(height), SWP_NOSENDCHANGING);

    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
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


#elif defined(PLATFORM_XCB)

/*------------------------------------------------------*/
//    Xcb
/*------------------------------------------------------*/

void Window::manageEvents()
{
    std::vector<xcb_generic_event_t*> generic_events;

    bool ignore_remaining_mouse_move_events = false;

    while(true)
    {
        xcb_generic_event_t* generic_event = xcb_poll_for_event(m_params.connection);

        if(generic_event)
        {
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
                    xcb_warp_pointer(m_params.connection, XCB_NONE, m_params.window, 0, 0, 0, 0, x, y);
                    xcb_flush(m_params.connection);
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
        case XCB_DESTROY_NOTIFY:
        {
            m_game_engine.onWindowDestroyEvent();
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
        throw err;
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

    m_params.connection = connection;
    m_params.window = window_id;
}

Window::~Window()
{
    xcb_disconnect(m_params.connection);
}

void Window::updateCursorPos()
{
    auto pointer_cookie = xcb_query_pointer(m_params.connection, m_params.window);
    xcb_generic_error_t* error = nullptr;
    auto reply = xcb_query_pointer_reply(m_params.connection, pointer_cookie, &error);
    m_input_state.cursor_pos.x = reply->win_x;
    m_input_state.cursor_pos.y = reply->win_y;
}

void Window::lockCursor()
{
    while(true)
    {
        const auto cookie = xcb_grab_pointer(m_params.connection, 1, m_params.window, XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, m_params.window, XCB_NONE, XCB_CURRENT_TIME);

        //TODO: ugly workaround
        xcb_generic_error_t* err = nullptr;
        const auto reply = xcb_grab_pointer_reply(m_params.connection, cookie, &err);
        if(reply->status == XCB_GRAB_STATUS_SUCCESS)
        {
            break;
        }
    }

    xcb_flush(m_params.connection);
    m_cursor_locked = true;
}

void Window::unlockCursor()
{
    xcb_ungrab_pointer(m_params.connection, XCB_CURRENT_TIME);
    xcb_flush(m_params.connection);
    m_cursor_locked = false;
}

void Window::show()
{
    const auto cookie = xcb_map_window_checked(m_params.connection, m_params.window);

    const auto check = xcb_request_check(m_params.connection, cookie);
    if(NULL != check)
    {
        std::stringstream ss;
        ss << "Failed to display window. Error: " << check;
        throw std::runtime_error(ss.str());
    }

    showCursor(false);
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

        xcb_cursor_t cursor = xcb_generate_id(m_params.connection);

        auto bitmap = xcb_create_pixmap_from_bitmap_data(m_params.connection, m_params.window, curs_windows_bits, 11, 19, 1, 0, 0, nullptr);
        auto mask = xcb_create_pixmap_from_bitmap_data(m_params.connection, m_params.window, mask_windows_bits, 11, 19, 1, 0, 0, nullptr);
        xcb_create_cursor(m_params.connection, cursor, bitmap, mask, 65535, 65535, 65535, 0, 0, 0, 0, 0);

        xcb_free_pixmap(m_params.connection, bitmap);
        xcb_free_pixmap(m_params.connection, mask);

        xcb_change_window_attributes(m_params.connection, m_params.window, XCB_CW_CURSOR, &cursor);
        xcb_flush(m_params.connection);

        xcb_free_cursor(m_params.connection, cursor);

        m_cursor_visible = true;
    }
    else if(!show && m_cursor_visible)
    {
        std::array<uint8_t, 8> curs_bits = {0, 0, 0, 0, 0, 0, 0, 0};

        xcb_cursor_t cursor = xcb_generate_id(m_params.connection);

        auto bitmap = xcb_create_pixmap_from_bitmap_data(m_params.connection, m_params.window, curs_bits.data(), 8, 8, 1, 0, 0, nullptr);
        xcb_create_cursor(m_params.connection, cursor, bitmap, bitmap, 0, 0, 0, 0, 0, 0, 0, 0);

        xcb_free_pixmap(m_params.connection, bitmap);

        xcb_change_window_attributes(m_params.connection, m_params.window, XCB_CW_CURSOR, &cursor);
        xcb_flush(m_params.connection);

        xcb_free_cursor(m_params.connection, cursor);

        m_cursor_visible = false;
    }
}

void Window::setSize(uint32_t width, uint32_t height)
{
    std::array<uint32_t, 4> values = {0, 0, width, height};

    uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    xcb_configure_window(m_params.connection, m_params.window, mask, values.data());
    xcb_flush(m_params.connection);
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

#endif
