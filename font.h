#ifndef FONT_H
#define FONT_H

#include <vector>
#include <string>
#include "geometry.h"

enum class FontType
{
    Basic,
    Count
};

struct glyph
{
    std::array<int32_t, 128> kerning{};

    int32_t bearing_x;
    int32_t descent;
    int32_t advance;

    uint8_t tex_id;
};

class Font
{
public:
    Font(const std::string& font_filename, uint32_t font_size);

    static constexpr size_t charCount() noexcept
    {
        return char_count;
    }

    std::vector<uint8_t>&& bitmaps() const;
    std::vector<uvec2> getMeasurments(const std::string& text) const;
    const glyph& glyphInfo(uint8_t c) const noexcept;

    uint32_t baselineDistance() const noexcept;
    uint32_t texWidth() const noexcept;
    uint32_t texHeight() const noexcept;
    uint32_t height() const noexcept;

private:
    static constexpr size_t first_char = 32;
    static constexpr size_t last_char = 126;
    static constexpr size_t char_count = last_char - first_char + 1;

    mutable std::vector<uint8_t> m_bitmaps;

    std::array<glyph, 128> m_glyphs;
    uint32_t m_baseline_distance;
    uint32_t m_font_height;
    uint32_t m_tex_width;
    uint32_t m_tex_height;
};

#endif // FONT_H
