#include "game_engine.h"
#include "game_utils.h"

#include <sstream>
#include <fstream>
#include <numeric>
#include <algorithm>

void GameEngine::setDefaultIni()
{
    m_init_params.res_x = 1280;
    m_init_params.res_y = 720;
    m_init_params.vsync = true;
}

void GameEngine::writeIni()
{
    std::ofstream ini(ini_filename);
    //TODO: check if file correctly opened
    ini << "vsync=" << (m_init_params.vsync ? "on" : "off") << std::endl;
    ini << "res_x=" << m_init_params.res_x << std::endl;
    ini << "res_y=" << m_init_params.res_y << std::endl;
}

void GameEngine::parseIni()
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

GameEngine::GameEngine()
{
    parseIni();

    //TODO: set application name
    const std::string app_name = "3dgame";
    const std::string font_directory = "assets/fonts/";

    /*create the application window*/
    m_window = std::make_unique<Window>(*this, app_name, m_init_params.res_x, m_init_params.res_y);

    /*create 3D graphics engine*/
    m_engine3d = std::make_unique<Engine3D>(*m_window, app_name);
    //TODO: vsync flag should just be passed to engine's constructor probably
    m_engine3d->enableVsync(m_init_params.vsync);

    m_fonts.emplace_back(font_directory + "font.ttf", 18);

    setupGui();

    m_scene = std::make_unique<Scene>(*m_engine3d, static_cast<float>(m_window->width()) / static_cast<float>(m_window->height()), m_fonts[0]);

    SceneInitData scene_init_data;

    for(const auto& font : m_fonts)
    {
        scene_init_data.fonts.push_back(&font);
    }

    m_engine3d->onSceneLoad(scene_init_data);

#if EDITOR_ENABLE
    m_editor = std::make_unique<Editor>(*m_window, *m_scene, *m_engine3d, m_fonts[0]);
#endif
}

void GameEngine::setupGui()
{
    m_fps_label = std::make_unique<Label>(*m_engine3d, 0, 0, m_fonts[0], "", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    m_fps_label->setUpdateCallback([this](Label& label)
    {
        static auto interval_start = std::chrono::steady_clock::now();
        static double dt = 0.0;

        auto curr_time = std::chrono::steady_clock::now();
        if(const auto t = (std::chrono::duration<double>(curr_time - interval_start)).count(); t >= 1.0)
        {
            interval_start = curr_time;
            dt = 1.0 / static_cast<double>(m_timer.getFps());
        }

        label.setText(std::format("Fps: {} | {:.2f}ms", m_timer.getFps(), dt * 1000.0));
    });

    m_console = std::make_unique<Console>(*m_engine3d, 0, 0, static_cast<float>(m_window->width()), 0.4f * static_cast<float>(m_window->height()), m_fonts[0], [&](const std::string& text)
    {
        processConsoleCmd(text);
    });
}

void GameEngine::processConsoleCmd(const std::string& text)
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
            if(m_engine3d->enableVsync(true))
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
            if(m_engine3d->enableVsync(false))
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

void GameEngine::run()
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
        m_window->manageEvents();

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
            m_editor->update(m_window->inputState(), frametime);
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

        m_fps_label->updateAndDraw(*m_engine3d, frametime);

        if(m_console_active)
        {
            m_console->updateAndDraw(*m_engine3d, frametime);
        }

        m_engine3d->updateAndRender(render_data, m_scene->camera());
    }
}

void GameEngine::stop() noexcept
{
    m_stop = true;
}

void GameEngine::onWindowDestroyEvent() noexcept
{
    stop();
}

void GameEngine::onWindowResizeEvent(uint32_t width, uint32_t height, float scale_x, float scale_y) noexcept
{
    if((0 == width) || (0 == height))
    {
        m_invalid_window = true;
    }
    else
    {
        m_invalid_window = false;

        m_gui_scale.x = static_cast<float>(width) / reference_resolution.x;
        m_gui_scale.y = static_cast<float>(height) / reference_resolution.y;

        m_scene->onWindowResize(static_cast<float>(width) / static_cast<float>(height));
        m_engine3d->onWindowResize(width, height);
#if EDITOR_ENABLE
        m_editor->onWindowResize(width, height, scale_x, scale_y);
#endif
    }
}

/*--------------------------------------- input handling -----------------------------------------*/

void GameEngine::onInputEvent(const Event& event, const InputState& input_state)
{
    if(EventType::KeyPressed == event.event)
    {
        if(VKeyTidle == event.key)
        {
            if(m_console_active)
            {
                m_console_active = false;
                m_console->setVisible(false);
                m_console->lostFocus();
            }
            else
            {
                m_console_active = true;
                m_console->setVisible(true);
                m_console->gotFocus();
            }

            return;
        }

        if(m_console_active)
        {
            m_console->onInputEvent(event, input_state);
            return;
        }

#if EDITOR_ENABLE
        if(event.key == VKeyF2)
        {
            m_edit_mode = !m_edit_mode;
            m_window->showCursor(m_edit_mode);
            return;
        }
#endif

        if(input_state.ctrl())
        {
            if(event.key == VKeyL)
            {
                m_window->toggleCursorLock();
                return;
            }

            if(event.key == VKeyQ)
            {
                stop();
                return;
            }
        }
    }

#if EDITOR_ENABLE
    if(m_edit_mode)
    {
        m_editor->onInputEvent(event, input_state);
    }
    else
#endif
    {
        m_scene->onInputEvent(event, input_state);
    }
}

void GameEngine::onControllerEvent(const ControllerEvent&, const ControllerState&)
{
}
