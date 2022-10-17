#include "point_light_edit_panel.h"
#include "rect.h"
#include "slider.h"
#include "button.h"
#include "checkbox.h"
#include <sstream>
#include <iomanip>

PointLightEditPanel::PointLightEditPanel(Engine3D& engine3d, float x, float y, Scene& scene, uint32_t point_light_id, const Font& font)
    : m_x(x)
    , m_y(y)
    , m_scene(scene)
    , m_point_light_id(point_light_id)
    , m_point_light(scene.staticPointLights()[point_light_id])
{
    const float padding_x = 0.05f * m_width;
    const float text_input_x = padding_x + 3.0f * static_cast<float>(font.texWidth()) + 10.0f;
    const float text_input_width = m_width - text_input_x - padding_x;
    const float slider_height = 16.0f;
    const float vertical_spacing = 7.0f;
    const float color_rect_size = 50.0f;
    const float checkbox_size = 20.0f;

    /*background rect*/
    m_children.push_back(std::make_unique<Rect>(engine3d, m_x, m_y, m_width, m_height, ColorRGBA(0.2f, 0.2f, 0.2f, 1.0f)));

    /*--- Position ---*/
    y = m_y + 0.1f * m_width;
    auto label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "Position", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    y += label->height() + 5.0f;
    m_children.push_back(std::move(label));

    /*X*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "x", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    auto text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.pos.x;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.pos.x = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    /*Y*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "y", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.pos.y;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.pos.y = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    /*Z*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "z", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.pos.z;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.pos.z = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    /*--- Color ---*/
    y += 5.0f;
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "Color", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
//    auto button = std::make_unique<Button>(engine3d, m_x + 0.5 * m_width, y, color_rect_size, color_rect_size, font, "", []()
//    {

//    });
//    button->setColor(m_point_light.color);
//    button->setHighlightColor(m_point_light.color);
//    button->setPressedColor(m_point_light.color);
//    button->setUpdateCallback([this](Button& b)
//    {
//        b.setColor(m_point_light.color);
//        b.setHighlightColor(m_point_light.color);
//        b.setPressedColor(m_point_light.color);
//    });
//    m_children.push_back(std::move(button));

//    y += color_rect_size + vertical_spacing;

    /*r*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "r", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.color.r;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.color.r = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    auto slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.color.r, [this](float value)
    {
        m_point_light.color.r = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.color.r)
        {
            slider.setValue(m_point_light.color.r);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    /*g*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "g", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.color.g;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.color.g = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.color.g, [this](float value)
    {
        m_point_light.color.g = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.color.g)
        {
            slider.setValue(m_point_light.color.g);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    /*b*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "b", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.color.b;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.color.b = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.color.b, [this](float value)
    {
        m_point_light.color.b = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.color.b)
        {
            slider.setValue(m_point_light.color.b);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    /*Power*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "power", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + 2.0f * text_input_x, y, text_input_width - text_input_x, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.power;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.power = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1000.0f, m_point_light.power, [this](float value)
    {
        m_point_light.power = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.power)
        {
            slider.setValue(m_point_light.power);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    /*Max Distance*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "max distance", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + padding_x + label->width() + 10, y, m_width - (padding_x + label->width() + 10) - padding_x, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.max_d;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.max_d = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 100.0f, m_point_light.max_d, [this](float value)
    {
        m_point_light.max_d = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.max_d)
        {
            slider.setValue(m_point_light.max_d);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    /*--- Attenuation ---*/
    y += 5.0f;
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "Attenuation", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));

    /*a0*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "a0", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.a0;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.a0 = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.a0, [this](float value)
    {
        m_point_light.a0 = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.a0)
        {
            slider.setValue(m_point_light.a0);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    /*a1*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "a1", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.a1;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.a1 = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.a1, [this](float value)
    {
        m_point_light.a1 = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.a1)
        {
            slider.setValue(m_point_light.a1);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    /*a2*/
    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "a2", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    text_input = std::make_unique<Label>(engine3d, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.a2;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.a2 = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;
    m_children.push_back(std::move(label));
    m_children.push_back(std::move(text_input));

    slider = std::make_unique<Slider<float>>(engine3d, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.a2, [this](float value)
    {
        m_point_light.a2 = value;
        updatePointLight();
    });
    slider->setUpdateCallback([this](Slider<float>& slider)
    {
        if(slider.value() != m_point_light.a2)
        {
            slider.setValue(m_point_light.a2);
        }
    });
    m_children.push_back(std::move(slider));
    y += slider_height + vertical_spacing;

    label = std::make_unique<Label>(engine3d, m_x + padding_x, y + 0.5f * checkbox_size, font, "Enable shadowmap", false, HorizontalAlignment::Left, VerticalAlignment::Center);
    m_children.push_back(std::move(label));

    auto checkbox = std::make_unique<Checkbox>(engine3d, m_x + m_width - padding_x - checkbox_size, y, checkbox_size, checkbox_size, m_point_light.shadow_map_res != 0);
    checkbox->setOnCheckCallback([this]()
    {
        m_point_light.shadow_map_res = 512;
        updatePointLight();
    });
    checkbox->setOnUncheckCallback([this]()
    {
        m_point_light.shadow_map_res = 0;
        updatePointLight();
    });
    m_children.push_back(std::move(checkbox));

    y += std::max(checkbox_size, 0.5f * checkbox_size + font.height()) + vertical_spacing;

    label = std::make_unique<Label>(engine3d, m_x + padding_x, y, font, "Shadowmap\nresolution", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    m_children.push_back(std::move(label));

    text_input = std::make_unique<Label>(engine3d, m_x + 2.0f * text_input_x, y, text_input_width - text_input_x, font.height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top);
    text_input->setCancalable(true);
    text_input->setUpdateCallback([this](Label& l)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_point_light.shadow_map_res;
        l.setText(ss.str());
    });
    text_input->setConfirmCallback([this](Label& l)
    {
        m_point_light.shadow_map_res = static_cast<uint32_t>(std::atoi(l.text().c_str()));
        updatePointLight();
    });
    m_children.push_back(std::move(text_input));

    for(auto& child : m_children)
    {
        child->setScissor({m_x, m_y, m_width, m_height});
    }
}

void PointLightEditPanel::update(Engine3D& engine3d, float dt)
{
    for(auto& child : m_children)
    {
        child->update(engine3d, dt);
    }
}

void PointLightEditPanel::draw(Engine3D& engine3d)
{
    for(auto& child : m_children)
    {
        child->draw(engine3d);
    }
}

bool PointLightEditPanel::isPointInside(vec2 p)
{
    const auto d = p - vec2(m_x, m_y);
    return (d.x > 0) && (d.x <= m_width) && (d.y > 0) && (d.y <= m_height);
}

void PointLightEditPanel::setScissor(Quad scissor)
{
    const Quad q = {m_x, m_y, m_width, m_height};
    m_scissor = quadOverlap(q, scissor);

    for(auto& child : m_children)
    {
        child->setScissor(*m_scissor);
    }
}

void PointLightEditPanel::onResolutionChange(float scale_x, float scale_y, const Font& font)
{

}

void PointLightEditPanel::onInputEvent(const Event& event, const InputState& input_state)
{
    determineFocus(m_children, event, input_state);
    forwardEventToFocused(event, input_state);
}

void PointLightEditPanel::updatePointLight()
{
    m_scene.updateStaticPointLight(m_point_light_id, m_point_light);
}
