#include "window.h"
#include "game_engine.h"
#include "game_utils.h"

#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <print>
#include "keycodes.h"

HWND Window::hwnd() const noexcept
{
    return m_hwnd;
}

HINSTANCE Window::hinstance() const noexcept
{
    return m_hinstance;
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

static Window* g_window = nullptr;

LRESULT CALLBACK WindowEventHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_window->EventHandler(hwnd, uMsg, wParam, lParam);
}

void Window::handleEvents()
{
    MSG msg;

    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

#if 0
   handleControllerEvents();
#endif
}

#if 0
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
        error(std::format("Error in XInputGetState - error code: {}", res));
    }
}
#endif

Window::Window(GameEngine& game_engine, std::string_view app_name, uint16_t width, uint16_t height)
    : m_x(0)
    , m_y(0)
    , m_width(width)
    , m_height(height)
    , m_game_engine(game_engine)
    , m_class_name(app_name)
{
    g_window = this;

    m_hinstance = GetModuleHandle(NULL);
    LPCTSTR window_name = m_class_name.c_str();
    LPCTSTR class_name = m_class_name.c_str();

    WNDCLASSEX wnd_class{};
    wnd_class.cbSize = sizeof(WNDCLASSEX);
    wnd_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wnd_class.lpszClassName = window_name;
    wnd_class.hInstance = m_hinstance;
    wnd_class.style = CS_HREDRAW | CS_VREDRAW;
    wnd_class.lpfnWndProc = WindowEventHandler;
    wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wnd_class);

    const int left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    m_hwnd = CreateWindow(class_name, window_name, WS_POPUP | WS_CLIPCHILDREN, left, top, width, height, NULL, NULL, m_hinstance, NULL);

    if(!m_hwnd)
    {
        //TODO: get error from GetLastError()
        error("Failed to create window.");
    }

    /*init raw input*/
    RAWINPUTDEVICE devices[2];

    //Mouse
    devices[0].usUsagePage = 0x01;
    devices[0].usUsage = 0x02;
    devices[0].dwFlags = 0;// RIDEV_NOLEGACY;
    devices[0].hwndTarget = m_hwnd;

    //Keyboard
    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x06;
    devices[1].dwFlags = RIDEV_NOLEGACY;
    devices[1].hwndTarget = m_hwnd;

    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
}

Window::~Window()
{
    UnregisterClass(m_class_name.c_str(), m_hinstance);
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
    track_mouse_event.hwndTrack = m_hwnd;
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

        m_input_state.caps_lock = GetKeyState(VK_CAPITAL) & 1;

        //Manage keyboard input
        if(raw.header.dwType == RIM_TYPEKEYBOARD)
        {
            Key key;

            if(VK_SHIFT == raw.data.keyboard.VKey)
            {
                if(raw.data.keyboard.MakeCode == 54)
                {
                    key = VKeyRShift;
                }
                else
                {
                    key = VKeyLShift;
                }
            }
            else if(VK_CONTROL == raw.data.keyboard.VKey)
            {
                if(raw.data.keyboard.Flags & RI_KEY_E0)
                {
                    key = VKeyRCtrl;
                }
                else
                {
                    key = VKeyLCtrl;
                }
            }
            else if(VK_MENU == raw.data.keyboard.VKey)
            {
                if(raw.data.keyboard.Flags & RI_KEY_E0)
                {
                    key = VKeyRAlt;
                }
                else
                {
                    key = VKeyLAlt;
                }
            }
            else
            {
                key = raw.data.keyboard.VKey;
            }

            if(raw.data.keyboard.Flags & RI_KEY_BREAK)
            {
                m_input_state.keyboard[key] = false;
                m_game_engine.onKeyReleased(key, m_input_state);
            }
            else
            {
                m_input_state.keyboard[key] = true;
                m_game_engine.onKeyPressed(key, m_input_state);
            }
        }
        //Manage mouse input
        else if(raw.header.dwType == RIM_TYPEMOUSE)
        {
            //TODO: add handling for more mouse buttons
            switch(raw.data.mouse.usButtonFlags)
            {
            case RI_MOUSE_LEFT_BUTTON_DOWN:
            {
                m_input_state.mouse |= LMB;
                m_game_engine.onMousePressed(LMB, m_input_state);
                break;
            }
            case RI_MOUSE_LEFT_BUTTON_UP:
            {
                m_input_state.mouse &= ~LMB;
                m_game_engine.onMouseReleased(LMB, m_input_state);
                break;
            }
            case RI_MOUSE_RIGHT_BUTTON_DOWN:
            {
                m_input_state.mouse |= RMB;
                m_game_engine.onMousePressed(RMB, m_input_state);
                break;
            }
            case RI_MOUSE_RIGHT_BUTTON_UP:
            {
                m_input_state.mouse &= ~RMB;
                m_game_engine.onMouseReleased(RMB, m_input_state);
                break;
            }
            case RI_MOUSE_MIDDLE_BUTTON_DOWN:
            {
                m_input_state.mouse |= MMB;
                m_game_engine.onMousePressed(MMB, m_input_state);
                break;
            }
            case RI_MOUSE_MIDDLE_BUTTON_UP:
            {
                m_input_state.mouse &= ~MMB;
                m_game_engine.onMouseReleased(MMB, m_input_state);
                break;
            }
            case RI_MOUSE_WHEEL:
            {
                const auto delta = static_cast<short>(raw.data.mouse.usButtonData);

                if(delta > 0)
                {
                    m_game_engine.onMouseScrolledUp(m_input_state);
                }
                else if(delta < 0)
                {
                    m_game_engine.onMouseScrolledDown(m_input_state);
                }

                break;
            }
            default:
                //handle mouse movement in WM_MOUSEMOVE event, because WM_INPUT SUCKS
                return 0;
            }
        }

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

        vec2 cursor_delta;

        if(m_cursor_left)
        {
            m_cursor_left = false;
            updateCursorPos();
            trackCursor();
            cursor_delta = vec2(0.0f, 0.0f);
        }
        else
        {
            cursor_delta = vec2(x, y) - m_input_state.cursor_pos;
        }

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

        if(wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))
        {
            m_game_engine.onMouseDragged(cursor_delta, m_input_state);
        }
        else
        {
            m_game_engine.onMouseMoved(cursor_delta, m_input_state);
        }

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
    ShowWindow(m_hwnd, SW_SHOW);
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

    SetWindowPos(m_hwnd, HWND_TOP, x, y, static_cast<int>(width), static_cast<int>(height), SWP_NOSENDCHANGING);

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
