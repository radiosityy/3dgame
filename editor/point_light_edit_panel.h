#ifndef POINT_LIGHT_EDIT_PANEL_H
#define POINT_LIGHT_EDIT_PANEL_H

#include "gui_object.h"
#include "light.h"
#include "label.h"
#include "scene.h"

class PointLightEditPanel : public GuiParentObject
{
public:
    PointLightEditPanel(Engine3D& engine3d, float x, float y, Scene& scene, uint32_t point_light_id, const Font& font);

    virtual void update(Engine3D& engine3d, float dt) override;
    virtual void draw(Engine3D& engine3d) override;
    virtual bool isPointInside(vec2) override;
    virtual void setScissor(Quad scissor) override;
    virtual void onInputEvent(const Event& event, const InputState& input_state) override;

private:
    void updatePointLight();

    float m_x;
    float m_y;
    const float m_width = 300.0f;
    const float m_height = 700.0f;

    Scene& m_scene;
    const uint32_t m_point_light_id;
    PointLight& m_point_light;

    std::vector<std::unique_ptr<GuiObject>> m_children;
    std::vector<Label*> m_text_inputs;

    std::optional<Quad> m_scissor;
};

#endif // POINT_LIGHT_EDIT_PANEL_H
