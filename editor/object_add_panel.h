#ifndef OBJECT_ADD_PANEL_H
#define OBJECT_ADD_PANEL_H

#include "gui_object.h"
#include "scene.h"
#include "button.h"

class ObjectAddPanel : public GuiParentObject
{
public:
    ObjectAddPanel(Engine3D& engine3d, float x, float y, Scene& scene, const Font& font);

    virtual bool onKeyPressedIntercept(Key, const InputState&) override;
    virtual void setScissor(Quad scissor) override;
    virtual bool isPointInside(vec2) override;
    virtual void onGotFocus() override;

private:
    void addObject(std::string_view mesh_filename);
    void updateList();

    Engine3D& m_engine3d;
    Scene& m_scene;
    const Font* m_font = nullptr;

    float m_x;
    float m_y;
    const float m_width = 400.0f;
    const float m_height = 400.0f;

    float m_list_y = 0.0f;

    Label* m_text_input = nullptr;
    std::vector<Button*> m_items;

    std::vector<std::string> m_mesh_filenames;
    int m_selected_item_id = -1;
};

#endif // OBJECT_ADD_PANEL_H
