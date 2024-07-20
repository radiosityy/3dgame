#include "object_add_panel.h"
#include "button.h"
#include <fstream>
#include <filesystem>
#include <print>

ObjectAddPanel::ObjectAddPanel(Renderer& renderer, float x, float y, Scene& scene, const Font& font)
    : m_renderer(renderer)
    , m_scene(scene)
    , m_font(&font)
    , m_x(x)
    , m_y(y)
{
    /*background rect*/
    addChild(std::make_unique<Rect>(m_renderer, m_x, m_y, m_width, m_height, ColorRGBA(0.2f, 0.2f, 0.2f, 1.0f)));

    /*top bar*/
    auto label = std::make_unique<Label>(m_renderer, m_x + 5.0f, m_y + 5.0f, font, "Pick mesh file", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    const float bar_height = 10.0f + label->height();
    addChild(std::make_unique<Rect>(m_renderer, m_x, m_y, m_width, bar_height, ColorRGBA(0.3f, 0.3f, 0.3f, 1.0f)));
    addChild(std::move(label));

    m_text_input = addChild(std::make_unique<Label>(m_renderer, m_x, m_y + bar_height, 0.7f * m_width, font.height(), font, "", true, HorizontalAlignment::Left, VerticalAlignment::Center));
    m_text_input->setBackgroundColor(ColorRGBA(0.1f, 0.1f, 0.1f, 1.0f));
    m_text_input->setConfirmCallback([&](Label& l){addObject(l.text());});
    m_text_input->setTextChangedCallback([this](std::string_view){updateList();});

    addChild(std::make_unique<Button>(m_renderer, m_x + 0.7f * m_width, m_y + bar_height, 0.3f * m_width, font.height(), font, "Select", [&](){addObject(m_text_input->text());}));

    m_list_y = m_y + bar_height + static_cast<float>(font.height());

    updateList();
}

bool ObjectAddPanel::onKeyPressedIntercept(Key key, const InputState&)
{
    switch(key)
    {
    case VKeyDown:
    case VKeyUp:
        if(!m_items.empty())
        {
            int new_selected_item_id = 0;

            if(key == VKeyUp)
            {
                if(m_selected_item_id == -1)
                {
                    new_selected_item_id = m_items.size() - 1;
                }
                else
                {
                    new_selected_item_id = m_selected_item_id - 1;
                }
            }
            else if(key == VKeyDown)
            {
                new_selected_item_id = (m_selected_item_id + 1) % m_items.size();
            }

            if(m_selected_item_id != new_selected_item_id)
            {
                resetMouseFocus();

                m_selected_item_id = new_selected_item_id;
                if(m_selected_item_id == -1)
                {
                    setKeyboardFocus(m_text_input);
                }
                else
                {
                    setKeyboardFocus(m_items[m_selected_item_id]);
                }
            }
        }
        return true;
    case VKeyEsc:
            m_selected_item_id = -1;
            setKeyboardFocus(m_text_input);
        return true;
    }

    return false;
}

void ObjectAddPanel::setScissor(Quad scissor)
{

}

bool ObjectAddPanel::isPointInside(vec2 p)
{
    const auto d = p - vec2(m_x, m_y);
    return (d.x > 0) && (d.x <= m_width) && (d.y > 0) && (d.y <= m_height);
}

void ObjectAddPanel::onGotFocus()
{
    setKeyboardFocus(m_text_input);
}

void ObjectAddPanel::addObject(std::string_view mesh_filename)
{
    const std::string mesh_file_path = std::string("assets/meshes/") + mesh_filename.data();
    const vec3 object_pos = m_scene.camera().pos() + 10.0f * m_scene.camera().forward();

    m_text_input->setText("");

    try
    {
        m_scene.addObject(m_renderer, mesh_file_path, RenderMode::Default, object_pos);
    }
    catch(std::exception& e)
    {
        //TODO: add some way of displaying errors in editor
        std::println("{}", e.what());
    }
}

void ObjectAddPanel::updateList()
{
    for(Button* b : m_items)
    {
        removeChild(b);
    }
    m_items.clear();

    float y = m_list_y;

    const auto text = m_text_input->text();

    for(const auto& entry : std::filesystem::directory_iterator("assets/meshes"))
    {
        const float item_height = static_cast<float>(m_font->height()) + 5.0f;
        const auto filename = entry.path().filename().string();

        if(filename.rfind(text, 0) != 0)
        {
            continue;
        }

        m_mesh_filenames.push_back(filename);

        const auto id = m_mesh_filenames.size() - 1;

        auto button = addChild(std::make_unique<Button>(m_renderer, m_x, y, m_width, item_height, *m_font, filename, [id, this]()
        {
            m_selected_item_id = -1;
            setKeyboardFocus(m_text_input);
            resetMouseFocus();

            m_text_input->setText(m_mesh_filenames[id]);
        },
            HorizontalAlignment::Left, VerticalAlignment::Center));

        button->setColor(ColorRGBA(0.2f, 0.2f, 0.2f, 1.0f));
        button->setHighlightColor(ColorRGBA(0.5f, 0.5f, 0.5f, 1.0f));

        m_items.emplace_back(button);

        y += item_height;
    }
}
