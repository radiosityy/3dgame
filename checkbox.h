#ifndef CHECKBOX_H
#define CHECKBOX_H

#include "rect.h"
#include "engine_3d.h"

class Checkbox : public Rect
{
public:
    Checkbox(Engine3D& engine3d, float x, float y, float w, float h, bool init_state = false);

    virtual void onInputEvent(const Event& event, const InputState& input_state) override;

    bool state() const;
    void setState(bool);
    void toggle();

    void setOnCheckCallback(std::function<void()>&& callback);
    void setOnUncheckCallback(std::function<void()>&& callback);

private:
    bool m_state = false;

    const TexId m_unchecked_tex;
    const TexId m_checked_tex;

    std::function<void()> m_on_check_callback;
    std::function<void()> m_on_uncheck_callback;
};

#endif //CHECKBOX_H
