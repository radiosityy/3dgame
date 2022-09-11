#ifndef COLORS_H
#define COLORS_H

//TODO: put glm in a dedicated header and include that
#include "geometry.h"

struct ColorRGBA
{
    ColorRGBA() = default;

    constexpr ColorRGBA(float r_, float g_, float b_, float a_ = 1.0f)
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_)
    {}

    constexpr ColorRGBA(float col, float a_ = 1.0f)
        : r(col)
        , g(col)
        , b(col)
        , a(a_)
    {}

    constexpr ColorRGBA(vec4 col)
        : r(col.r)
        , g(col.g)
        , b(col.b)
        , a(col.a)
    {}

    constexpr ColorRGBA(vec3 col, float a_ = 1.0f)
        : r(col.r)
        , g(col.g)
        , b(col.b)
        , a(a_)
    {}

    static constexpr ColorRGBA fromInts(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255.0f)
    {
        return ColorRGBA(static_cast<float>(r_) / 255.0f,
                         static_cast<float>(g_) / 255.0f,
                         static_cast<float>(b_) / 255.0f,
                         static_cast<float>(a_) / 255.0f);
    }

    operator vec3() const
    {
        return vec3(r, g, b);
    }

    operator vec4() const
    {
        return vec4(r, g, b, a);
    }

    float r;
    float g;
    float b;
    float a;

    const static ColorRGBA White;
    const static ColorRGBA Black;
    const static ColorRGBA Red;
    const static ColorRGBA Green;
    const static ColorRGBA Blue;

    const static ColorRGBA UiBackgroundColor;
    const static ColorRGBA FontColor;
};


inline const ColorRGBA ColorRGBA::White  = {1.0f, 1.0f, 1.0f, 1.0f};
inline const ColorRGBA ColorRGBA::Black  = {0.0f, 0.0f, 0.0f, 1.0f};
inline const ColorRGBA ColorRGBA::Red    = {1.0f, 0.0f, 0.0f, 1.0f};
inline const ColorRGBA ColorRGBA::Green  = {0.0f, 1.0f, 0.0f, 1.0f};
inline const ColorRGBA ColorRGBA::Blue   = {0.0f, 0.0f, 1.0f, 1.0f};

inline const ColorRGBA ColorRGBA::UiBackgroundColor = ColorRGBA::White;
inline const ColorRGBA ColorRGBA::FontColor = ColorRGBA::White;

#endif // COLORS_H
