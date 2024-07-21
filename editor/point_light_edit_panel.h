#ifndef POINT_LIGHT_EDIT_PANEL_H
#define POINT_LIGHT_EDIT_PANEL_H

#include "gui_object.h"
#include "light.h"
#include "label.h"
#include "slider.h"
#include "scene.h"

class PointLightEditPanel : public GuiParentObject
{
public:
    PointLightEditPanel(Renderer& renderer, float x, float y, Scene& scene, uint32_t point_light_id, const Font& font);

    virtual void update(Renderer&) override;
    virtual bool isPointInside(vec2) override;
    virtual void setScissor(Quad scissor) override;

private:
    void updatePointLight();

    float m_x;
    float m_y;
    const float m_width = 300.0f;
    const float m_height = 700.0f;

    Scene& m_scene;
    const uint32_t m_point_light_id;
    PointLight& m_point_light;

    Label* m_text_input_x = nullptr;
    Label* m_text_input_y = nullptr;
    Label* m_text_input_z = nullptr;
    Label* m_text_input_r = nullptr;
    Slider<float>* m_slider_r = nullptr;
    Label* m_text_input_g = nullptr;
    Slider<float>* m_slider_g = nullptr;
    Label* m_text_input_b = nullptr;
    Slider<float>* m_slider_b = nullptr;
    Label* m_text_input_power = nullptr;
    Slider<float>* m_slider_power = nullptr;
    Label* m_text_input_max_d = nullptr;
    Slider<float>* m_slider_max_d = nullptr;
    Label* m_text_input_a0 = nullptr;
    Slider<float>* m_slider_a0 = nullptr;
    Label* m_text_input_a1 = nullptr;
    Slider<float>* m_slider_a1 = nullptr;
    Label* m_text_input_a2 = nullptr;
    Slider<float>* m_slider_a2 = nullptr;
    Label* m_text_input_shadow_map_res = nullptr;

    std::optional<Quad> m_scissor;
};

#endif // POINT_LIGHT_EDIT_PANEL_H
