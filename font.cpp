#include "font.h"
#include <print>

#include <ft2build.h>
#include FT_FREETYPE_H

Font::Font(const std::string& font_filename, uint32_t font_size)
{
    FT_Library library;
    FT_Face face;

    auto error = FT_Init_FreeType(&library);
    //TODO: improve error handling here
    if(error)
    {
        std::println("{}", error);
        throw error;
    }

    error = FT_New_Face(library, font_filename.c_str(), 0, &face);
    if(error)
    {
        std::println("{}", error);
        throw error;
    }

    error = FT_Set_Pixel_Sizes(face, 0, font_size);
    if(error)
    {
        std::println("{}", error);
        throw error;
    }

    m_baseline_distance = face->size->metrics.height / 64;

    m_tex_width = 0;
    m_tex_height = 0;
    int min_y = 0;
    int max_y = 0;

    std::vector<std::pair<std::vector<uint8_t>, std::array<uint32_t, 2>>> bitmaps(char_count);

    for(size_t c = 0; c < char_count; c++)
    {
        const size_t charcode = first_char + c;

        error = FT_Load_Char(face, charcode, FT_LOAD_RENDER);
        if(error)
        {
            std::println("{}", error);
            throw error;
        }

        const auto w = face->glyph->bitmap.width;
        const auto h = face->glyph->bitmap.rows;

        bitmaps[c].first.resize(w*h);
        bitmaps[c].second[0] = w;
        bitmaps[c].second[1] = h;

        m_tex_width = std::max(w, m_tex_width);
        m_tex_height = std::max(h, m_tex_height);

        uint8_t* src = face->glyph->bitmap.buffer;
        uint8_t* startOfLine = src;
        size_t dst = 0;

        for(size_t y = 0; y < h; y++)
        {
            src = startOfLine;

            for(size_t x = 0; x < w; x++)
            {
                auto value = *src;
                src++;

                bitmaps[c].first[dst++] = value;
            }

            startOfLine += face->glyph->bitmap.pitch;
        }

        const auto descent = (face->glyph->metrics.height - face->glyph->metrics.horiBearingY) / 64;

        m_glyphs[charcode].tex_id = c;
        m_glyphs[charcode].bearing_x = face->glyph->metrics.horiBearingX / 64;
        m_glyphs[charcode].descent = descent;
        m_glyphs[charcode].advance = face->glyph->advance.x / 64;

        min_y = std::min<int>(min_y, descent);
        max_y = std::max<int>(max_y, face->glyph->metrics.horiBearingY / 64);

        if(FT_HAS_KERNING(face))
        {
            const auto glyph_index = FT_Get_Char_Index(face, charcode);

            for(size_t r = 0; r < char_count; r++)
            {
                const auto right_charcode = first_char + r;
                const auto right_glyph_index = FT_Get_Char_Index(face, right_charcode);

                FT_Vector kern;
                error = FT_Get_Kerning(face, glyph_index, right_glyph_index, FT_KERNING_DEFAULT, &kern);
                if(error)
                {
                    std::println("{}", error);
                    throw error;
                }

                m_glyphs[charcode].kerning[right_charcode] = kern.x / 64;
            }
        }
    }

    m_font_height = max_y - min_y;

    /*create bitmaps with equal sizes*/
    m_bitmaps.resize(char_count * m_tex_width * m_tex_height);

    size_t offset = 0;
    for(size_t i = 0; i < bitmaps.size(); i++)
    {
        size_t bitmap_offset = 0;

        for(size_t y = m_tex_height; y > 0; y--)
        {
            for(size_t x = 0; x < m_tex_width; x++)
            {
                if((y <= bitmaps[i].second[1]) && (x < bitmaps[i].second[0]))
                {
                    m_bitmaps[offset++]  = bitmaps[i].first[bitmap_offset++];
                }
                else
                {
                    m_bitmaps[offset++] = 0;
                }
            }
        }
    }
}

std::vector<uint8_t>&& Font::bitmaps() const
{
    return std::move(m_bitmaps);
}

std::vector<uvec2> Font::getMeasurments(const std::string& text) const
{
    std::vector<uvec2> line_measurements = {{0, m_font_height}};

    for(size_t i = 0; i < text.size(); i++)
    {
        const auto c = text[i];

        if('\n' == c)
        {
            line_measurements.emplace_back(0, m_font_height);
            continue;
        }

        auto& g = glyphInfo(c);
        const float a = g.advance;
        const float k = (i < text.size() - 1) ? g.kerning[text[i+1]] : 0;

        line_measurements.back().x += a + k;
    }

    return line_measurements;
}

const glyph&Font::glyphInfo(uint8_t c) const noexcept
{
    return m_glyphs[c];
}

uint32_t Font::baselineDistance() const noexcept
{
    return m_baseline_distance;
}

uint32_t Font::texWidth() const noexcept
{
    return m_tex_width;
}

uint32_t Font::texHeight() const noexcept
{
    return m_tex_height;
}

uint32_t Font::height() const noexcept
{
    return m_font_height;
}
