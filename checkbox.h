#ifndef CHECKBOX_H
#define CHECKBOX_H

#include "rect.h"
#include "renderer.h"

class Checkbox : public Rect
{
public:
    Checkbox(Renderer& renderer, float x, float y, float w, float h, bool init_checked = false);

    virtual void onMouseReleased(MouseButton, const InputState&, bool) override;

    bool state() const;
    void setChecked(bool);
    void toggle();

    void setOnCheckCallback(std::move_only_function<void()>&& callback);
    void setOnUncheckCallback(std::move_only_function<void()>&& callback);

private:
    bool m_checked = false;

    const TexId m_unchecked_tex;
    const TexId m_checked_tex;

    std::move_only_function<void()> m_on_check_callback;
    std::move_only_function<void()> m_on_uncheck_callback;
};

#endif //CHECKBOX_H
