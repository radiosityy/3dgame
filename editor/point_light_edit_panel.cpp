#include "point_light_edit_panel.h"
#include "rect.h"
#include "button.h"
#include "checkbox.h"
#include <sstream>
#include <iomanip>

PointLightEditPanel::PointLightEditPanel(Renderer& renderer, float x, float y, Scene& scene, uint32_t point_light_id, const Font& font)
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
    addChild(std::make_unique<Rect>(renderer, m_x, m_y, m_width, m_height, ColorRGBA(0.2f, 0.2f, 0.2f, 1.0f)));

    /*--- Position ---*/
    y = m_y + 0.1f * m_width;
    auto label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "Position", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    y += label->height() + 5.0f;

    /*X*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "x", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_x = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_x->setCancalable(true);
    m_text_input_x->setConfirmCallback([this](Label& l)
    {
        m_point_light.pos.x = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    /*Y*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "y", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_y = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_y->setCancalable(true);
    m_text_input_y->setConfirmCallback([this](Label& l)
    {
        m_point_light.pos.y = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    /*Z*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "z", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_z = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_z->setCancalable(true);
    m_text_input_z->setConfirmCallback([this](Label& l)
    {
        m_point_light.pos.z = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    /*--- Color ---*/
    y += 5.0f;
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "Color", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    y += label->height() + vertical_spacing;
//    auto button = addChild(std::make_unique<Button>(renderer, m_x + 0.5 * m_width, y, color_rect_size, color_rect_size, font, "", []()
//    {

//    }));
//    button->setColor(m_point_light.color);
//    button->setHighlightColor(m_point_light.color);
//    button->setPressedColor(m_point_light.color);
//    button->setUpdateCallback([this](Button& b)
//    {
//        b.setColor(m_point_light.color);
//        b.setHighlightColor(m_point_light.color);
//        b.setPressedColor(m_point_light.color);
//    });

//    y += color_rect_size + vertical_spacing;

    /*r*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "r", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_r = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_r->setCancalable(true);
    m_text_input_r->setConfirmCallback([this](Label& l)
    {
        m_point_light.color.r = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_r = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.color.r, [this](float value)
    {
        m_point_light.color.r = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    /*g*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "g", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_g = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_g->setCancalable(true);
    m_text_input_g->setConfirmCallback([this](Label& l)
    {
        m_point_light.color.g = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_g = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.color.g, [this](float value)
    {
        m_point_light.color.g = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    /*b*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "b", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_b = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_b->setCancalable(true);
    m_text_input_b->setConfirmCallback([this](Label& l)
    {
        m_point_light.color.b = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_b = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.color.b, [this](float value)
    {
        m_point_light.color.b = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    /*Power*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "power", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_power = addChild(std::make_unique<Label>(renderer, m_x + 2.0f * text_input_x, y, text_input_width - text_input_x, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_power->setCancalable(true);
    m_text_input_power->setConfirmCallback([this](Label& l)
    {
        m_point_light.power = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_power = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1000.0f, m_point_light.power, [this](float value)
    {
        m_point_light.power = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    /*Max Distance*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "max distance", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_max_d = addChild(std::make_unique<Label>(renderer, m_x + padding_x + label->width() + 10, y, m_width - (padding_x + label->width() + 10) - padding_x, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_max_d->setCancalable(true);
    m_text_input_max_d->setConfirmCallback([this](Label& l)
    {
        m_point_light.max_d = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_max_d = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 100.0f, m_point_light.max_d, [this](float value)
    {
        m_point_light.max_d = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    /*--- Attenuation ---*/
    y += 5.0f;
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "Attenuation", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    y += label->height() + vertical_spacing;

    /*a0*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "a0", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_a0 = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_a0->setCancalable(true);
    m_text_input_a0->setConfirmCallback([this](Label& l)
    {
        m_point_light.a0 = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_a0 = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.a0, [this](float value)
    {
        m_point_light.a0 = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    /*a1*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "a1", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_a1 = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_a1->setCancalable(true);
    m_text_input_a1->setConfirmCallback([this](Label& l)
    {
        m_point_light.a1 = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_a1 = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.a1, [this](float value)
    {
        m_point_light.a1 = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    /*a2*/
    label = addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "a2", false, HorizontalAlignment::Left, VerticalAlignment::Top));
    m_text_input_a2 = addChild(std::make_unique<Label>(renderer, m_x + text_input_x, y, text_input_width, label->height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_a2->setCancalable(true);
    m_text_input_a2->setConfirmCallback([this](Label& l)
    {
        m_point_light.a2 = static_cast<float>(std::atof(l.text().c_str()));
        updatePointLight();
    });
    y += label->height() + vertical_spacing;

    m_slider_a2 = addChild(std::make_unique<Slider<float>>(renderer, m_x + padding_x, y, m_width - 2.0f * padding_x, slider_height, 0.0f, 1.0f, m_point_light.a2, [this](float value)
    {
        m_point_light.a2 = value;
        updatePointLight();
    }));
    y += slider_height + vertical_spacing;

    addChild(std::make_unique<Label>(renderer, m_x + padding_x, y + 0.5f * checkbox_size, font, "Enable shadowmap", false, HorizontalAlignment::Left, VerticalAlignment::Center));

    auto checkbox = addChild(std::make_unique<Checkbox>(renderer, m_x + m_width - padding_x - checkbox_size, y, checkbox_size, checkbox_size, m_point_light.shadow_map_res != 0));
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

    y += std::max(checkbox_size, 0.5f * checkbox_size + font.height()) + vertical_spacing;

    addChild(std::make_unique<Label>(renderer, m_x + padding_x, y, font, "Shadowmap\nresolution", false, HorizontalAlignment::Left, VerticalAlignment::Top));

    m_text_input_shadow_map_res = addChild(std::make_unique<Label>(renderer, m_x + 2.0f * text_input_x, y, text_input_width - text_input_x, font.height(), font, "", true, HorizontalAlignment::Center, VerticalAlignment::Top));
    m_text_input_shadow_map_res->setCancalable(true);
    m_text_input_shadow_map_res->setConfirmCallback([this](Label& l)
    {
        m_point_light.shadow_map_res = static_cast<uint32_t>(std::atoi(l.text().c_str()));
        updatePointLight();
    });

    //TODO:
    // for(auto& child : m_children)
    // {
    //     child->setScissor({m_x, m_y, m_width, m_height});
    // }
}

void PointLightEditPanel::update(Renderer& renderer)
{
    m_text_input_x->setText(std::format("{:.2f}", m_point_light.pos.x));
    m_text_input_y->setText(std::format("{:.2f}", m_point_light.pos.y));
    m_text_input_z->setText(std::format("{:.2f}", m_point_light.pos.z));

    m_text_input_r->setText(std::format("{:.2f}", m_point_light.color.r));
    if(m_slider_r->value() != m_point_light.color.r)
    {
        m_slider_r->setValue(m_point_light.color.r);
    }

    m_text_input_g->setText(std::format("{:.2f}", m_point_light.color.g));
    if(m_slider_g->value() != m_point_light.color.g)
    {
        m_slider_g->setValue(m_point_light.color.g);
    }

    m_text_input_b->setText(std::format("{:.2f}", m_point_light.color.b));
    if(m_slider_b->value() != m_point_light.color.b)
    {
        m_slider_b->setValue(m_point_light.color.b);
    }

    m_text_input_power->setText(std::format("{:.2f}", m_point_light.power));
    if(m_slider_power->value() != m_point_light.power)
    {
        m_slider_power->setValue(m_point_light.power);
    }

    m_text_input_max_d->setText(std::format("{:.2f}", m_point_light.max_d));
    if(m_slider_max_d->value() != m_point_light.max_d)
    {
        m_slider_max_d->setValue(m_point_light.max_d);
    }

    m_text_input_a0->setText(std::format("{:.2f}", m_point_light.a0));
    if(m_slider_a0->value() != m_point_light.a0)
    {
        m_slider_a0->setValue(m_point_light.a0);
    }

    m_text_input_a1->setText(std::format("{:.2f}", m_point_light.a1));
    if(m_slider_a1->value() != m_point_light.a1)
    {
        m_slider_a1->setValue(m_point_light.a1);
    }

    m_text_input_a2->setText(std::format("{:.2f}", m_point_light.a2));
    if(m_slider_a2->value() != m_point_light.a2)
    {
        m_slider_a2->setValue(m_point_light.a2);
    }

    m_text_input_shadow_map_res->setText(std::format("{}", m_point_light.shadow_map_res));

    updateChildren(renderer);
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

    //TODO:
    // for(auto& child : m_children)
    // {
    //     child->setScissor(*m_scissor);
    // }
}

void PointLightEditPanel::updatePointLight()
{
    m_scene.updateStaticPointLight(m_point_light_id, m_point_light);
}
