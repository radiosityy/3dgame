#include "game.h"
#include "game_utils.h"

#include <sstream>
#include <fstream>
#include <numeric>
#include <algorithm>

void Game::setDefaultIni()
{
    m_init_params.res_x = 1280;
    m_init_params.res_y = 720;
    m_init_params.vsync = true;
}

void Game::writeIni()
{
    std::ofstream ini(ini_filename);
    //TODO: check if file correctly opened
    ini << "vsync=" << (m_init_params.vsync ? "on" : "off") << std::endl;
    ini << "res_x=" << m_init_params.res_x << std::endl;
    ini << "res_y=" << m_init_params.res_y << std::endl;
}

void Game::parseIni()
{
    setDefaultIni();

    std::ifstream ini(ini_filename);

    if(ini)
    {
        //TODO: check if file correctly opened
//        if(!ini)
//        {
//            error(std::format("Failed to create ini file: {}", ini.bad() ? std::strerror(errno) : "Unknown error."));
//        }

        uint32_t line_num = 1;
        for(std::array<char, 256> line; ini.getline(line.data(), line.size()); line_num++)
        {
            //TODO: check if line was successfully got
            const std::string_view s(line.data());

            auto pos = s.find('=');

            if((pos == std::string_view::npos) || (pos == 0) || (pos == s.size() - 1))
            {
                log(std::format("Invalid entry in ini file (line {}): {}", line_num, s));
                continue;
            }

            const std::string_view param = s.substr(0, pos);
            const std::string_view value = s.substr(pos + 1);

            if(param == "vsync")
            {
                if(value == "on")
                {
                    m_init_params.vsync = true;
                }
                else if(value == "off")
                {
                    m_init_params.vsync = false;
                }
            }
            else if(param == "res_x")
            {
                m_init_params.res_x = std::stoul(std::string(value));
                if(m_init_params.res_x == 0)
                {
    //                invalid_value_error();
                }
            }
            else if(param == "res_y")
            {
                m_init_params.res_y = std::stoul(std::string(value));
                if(m_init_params.res_y == 0)
                {
    //                invalid_value_error();
                }
            }
        }

        ini.close();
    }

    writeIni();
}

Game::Game()
{
    parseIni();

    //TODO: set application name
    const std::string app_name = "3dgame";
    const std::string font_directory = "assets/fonts/";

    m_window = std::make_unique<Window>(*this, app_name, m_init_params.res_x, m_init_params.res_y);

    m_renderer = std::make_unique<Renderer>(*m_window, app_name);
    //TODO: vsync flag should just be passed to engine's constructor probably
    m_renderer->enableVsync(m_init_params.vsync);

    m_fonts.emplace_back(font_directory + "font.ttf", 18);

    m_fps_label = std::make_unique<Label>(*m_renderer, 0, 0, m_fonts[0], "", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    m_console = std::make_unique<Console>(*m_renderer, 0, 0, static_cast<float>(m_window->width()), 0.4f * static_cast<float>(m_window->height()), m_fonts[0], [&](const std::string& text)
    {
        processConsoleCmd(text);
    });

    m_scene = std::make_unique<Scene>(*m_renderer, static_cast<float>(m_window->width()) / static_cast<float>(m_window->height()), m_fonts[0]);

    SceneInitData scene_init_data;

    for(const auto& font : m_fonts)
    {
        scene_init_data.fonts.push_back(&font);
    }

    m_renderer->onSceneLoad(scene_init_data);

#if EDITOR_ENABLE
    m_editor = std::make_unique<Editor>(m_window->width(), m_window->height(), *m_scene, *m_renderer, m_fonts[0]);
#endif
}

void Game::updateFpsLabel()
{
    static auto interval_start = std::chrono::steady_clock::now();
    static double dt = 0.0;

    auto curr_time = std::chrono::steady_clock::now();
    if(const auto t = (std::chrono::duration<double>(curr_time - interval_start)).count(); t >= 1.0)
    {
        interval_start = curr_time;
        dt = 1.0 / static_cast<double>(m_timer.getFps());
    }

    m_fps_label->setText(std::format("Fps: {} | {:.2f}ms", m_timer.getFps(), dt * 1000.0));
}

void Game::processConsoleCmd(const std::string& text)
{
    /*split command string into individual words*/
    std::vector<std::string> words;

    size_t start = 0;
    auto end = text.find(' ');

    while(end != std::string::npos)
    {
        words.emplace_back(text.substr(start, end - start));
        start = end + 1;
        end = text.find(' ', start);
    }

    words.emplace_back(text.substr(start, text.size()));

    if(words.empty() || words[0].empty())
    {
        m_console->print("");
        return;
    }

    /*parse command words*/
    if("res" == words[0])
    {
        if(words.size() != 3)
        {
            m_console->print("res: command expects exactly 2 arguments.");
            return;
        }

        const uint32_t w = static_cast<uint32_t>(std::atoi(words[1].c_str()));
        const uint32_t h = static_cast<uint32_t>(std::atoi(words[2].c_str()));

        m_window->setSize(w, h);
        m_console->print(std::format("Window resized to {}x{}", w, h));
        return;
    }

    if("exit" == words[0])
    {
        stop();
        return;
    }

    if("vsync" == words[0])
    {
        if(words.size() != 2)
        {
            m_console->print("vsync: command expects exactly 1 argument.");
            return;
        }

        if("on" == words[1])
        {
            if(m_renderer->enableVsync(true))
            {
                m_console->print("Vsync enabled.");
            }
            else
            {
                m_console->print("Failed to enable vsync.");
            }
        }
        else if("off" == words[1])
        {
            if(m_renderer->enableVsync(false))
            {
                m_console->print("Vsync disabled.");
            }
            else
            {
                m_console->print("Failed to disable vsync.");
            }
        }
        else
        {
            m_console->print("vsync: unknown argument - \"" + words[1] + "\"");
        }

        return;
    }

    m_console->print(words[0] + ": unknown command.");
}

void Game::run()
{
    m_window->show();
    m_window->lockCursor();
    m_window->showCursor(false);

#if EDITOR_ENABLE
    m_edit_mode = true;
    m_window->showCursor(true);
#endif

    m_timer.reset();

    while(true)
    {
        m_window->handleEvents();

        if(m_stop)
        {
            break;
        }

        if(m_invalid_window || !m_window->hasFocus())
        {
            continue;
        }

        m_timer.tick();

        const float frametime = m_timer.getDeltaTime();
        float sim_frametime = std::min(frametime, max_frametime);

#if EDITOR_ENABLE
        if(m_edit_mode)
        {
            m_editor->update(m_window->inputState(), frametime, !m_console_active);
        }
        else
#endif
        {
            while(true)
            {
                if(sim_frametime > dt)
                {
                    m_scene->update(dt, m_window->inputState());
                    sim_frametime -= dt;
                }
                else
                {
                    m_scene->update(sim_frametime, m_window->inputState());
                    break;
                }
            }
        }

        RenderData render_data;
        m_scene->draw(render_data);
#if EDITOR_ENABLE
        if(m_edit_mode)
        {
            m_editor->draw(render_data);
        }
#endif

        updateFpsLabel();
        m_fps_label->update(*m_renderer);
        m_fps_label->draw(*m_renderer);

        if(m_console_active)
        {
            m_console->update(*m_renderer);
            m_console->draw(*m_renderer);
        }

        m_renderer->updateAndRender(render_data, m_editor->camera());
    }
}

void Game::stop() noexcept
{
    m_stop = true;
}

void Game::onWindowDestroy() noexcept
{
    stop();
}

void Game::onWindowResize(uint32_t width, uint32_t height) noexcept
{
    if((0 == width) || (0 == height))
    {
        m_invalid_window = true;
    }
    else
    {
        m_invalid_window = false;

        m_console->setSize(width, 0.4f * height);

        m_scene->onWindowResize(static_cast<float>(width) / static_cast<float>(height));
        m_renderer->onWindowResize(width, height);
#if EDITOR_ENABLE
        m_editor->onWindowResize(width, height);
#endif
    }
}

/*--------------------------------------- input handling -----------------------------------------*/

void Game::onKeyPressed(Key key, const InputState& input_state)
{
    if(input_state.ctrl())
    {
        if(key == VKeyL)
        {
            m_window->toggleCursorLock();
            return;
        }

        if(key == VKeyQ)
        {
            stop();
            return;
        }
    }

    if(VKeyTidle == key)
    {
        if(m_console_active)
        {
            m_console_active = false;
            m_console->setVisible(false);
            m_console->onLostFocus();
        }
        else
        {
            m_console_active = true;
            m_console->setVisible(true);
            m_console->onGotFocus();
        }

        return;
    }

    if(m_console_active)
    {
        m_console->onKeyPressed(key, input_state);
        return;
    }

#if EDITOR_ENABLE
    if(key == VKeyF2)
    {
        m_edit_mode = !m_edit_mode;
        m_window->showCursor(m_edit_mode);
        return;
    }
#endif

#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onKeyPressed(key, input_state);
    }
    else
#endif
    {
        m_scene->onKeyPressed(key, input_state);
    }
}

void Game::onKeyReleased(Key key, const InputState& input_state)
{
#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onKeyReleased(key, input_state);
    }
    else
#endif
    {
        // m_scene->onKeyReleased(key, input_state);
    }
}

void Game::onMousePressed(MouseButton mb, const InputState& input_state)
{
    if(m_console_active)
    {
        m_console->onMousePressed(mb, input_state);
        return;
    }

#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onMousePressed(mb, input_state);
    }
    else
#endif
    {
        // m_scene->onMousePressed(mb, input_state);
    }
}

void Game::onMouseReleased(MouseButton mb, const InputState& input_state)
{
#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onMouseReleased(mb, input_state, true);
    }
    else
#endif
    {
        // m_scene->onMouseReleased(mb, input_state);
    }
}

void Game::onMouseMoved(vec2 cursor_delta, const InputState& input_state)
{
#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onMouseMoved(cursor_delta, input_state);
    }
    else
#endif
    {
        m_scene->onMouseMoved(cursor_delta, input_state);
    }
}

void Game::onMouseScrolledUp(const InputState& input_state)
{
#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onMouseScrolledUp(input_state);
    }
    else
#endif
    {
        m_scene->onMouseScrolledUp(input_state);
    }
}

void Game::onMouseScrolledDown(const InputState& input_state)
{
#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onMouseScrolledDown(input_state);
    }
    else
#endif
    {
        m_scene->onMouseScrolledDown(input_state);
    }
}
