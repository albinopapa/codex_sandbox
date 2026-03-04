#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <algorithm>

#include "color.hpp"
#include "math.hpp"
#include "surface.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace gfx {

struct FontGlyph {
    std::size_t u{};
    std::size_t v{};
    std::size_t width{};
    std::size_t height{};
    int advance{};
};

class Font {
public:
    static Font createAtlas(std::string_view familyName, int pixelHeight) {
        constexpr std::size_t firstChar = 32;
        constexpr std::size_t lastChar = 126;
        constexpr std::size_t glyphCount = lastChar - firstChar + 1;

        const std::size_t cellWidth = static_cast<std::size_t>(pixelHeight / 2 + 6);
        const std::size_t cellHeight = static_cast<std::size_t>(pixelHeight + 6);
        constexpr std::size_t columns = 16;
        const std::size_t rows = (glyphCount + columns - 1) / columns;

        Font font;
        font.lineHeight_ = static_cast<std::size_t>(pixelHeight + 2);
        font.atlas_ = Surface<Color8>(columns * cellWidth, rows * cellHeight, Color8{0, 0, 0, 0});

#ifdef _WIN32
        HDC dc = CreateCompatibleDC(nullptr);
        if (dc != nullptr) {
            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = static_cast<LONG>(font.atlas_.width());
            bmi.bmiHeader.biHeight = -static_cast<LONG>(font.atlas_.height());
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            void* bits = nullptr;
            HBITMAP bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
            if (bitmap != nullptr) {
                HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
                SetBkMode(dc, OPAQUE);
                SetBkColor(dc, RGB(0, 0, 0));
                SetTextColor(dc, RGB(255, 255, 255));

                std::wstring wideFamily(familyName.begin(), familyName.end());
                HFONT hfont = CreateFontW(
                    pixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    wideFamily.empty() ? L"Consolas" : wideFamily.c_str());

                if (hfont != nullptr) {
                    HGDIOBJ oldFont = SelectObject(dc, hfont);
                    RECT clearRect{0, 0, static_cast<LONG>(font.atlas_.width()), static_cast<LONG>(font.atlas_.height())};
                    FillRect(dc, &clearRect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

                    for (std::size_t i = 0; i < glyphCount; ++i) {
                        const char ch = static_cast<char>(firstChar + i);
                        const std::size_t col = i % columns;
                        const std::size_t row = i / columns;
                        const int ox = static_cast<int>(col * cellWidth + 2);
                        const int oy = static_cast<int>(row * cellHeight + 2);

                        char glyph[2] = {ch, '\0'};
                        TextOutA(dc, ox, oy, glyph, 1);

                        SIZE textSize{};
                        GetTextExtentPoint32A(dc, glyph, 1, &textSize);

                        font.glyphs_[static_cast<std::size_t>(static_cast<unsigned char>(ch))] = {
                            static_cast<std::size_t>(ox),
                            static_cast<std::size_t>(oy),
                            static_cast<std::size_t>(cellWidth - 2),
                            static_cast<std::size_t>(cellHeight - 2),
                            textSize.cx > 0 ? textSize.cx : static_cast<int>(cellWidth - 2)
                        };
                    }

                    const auto* rgba = static_cast<const std::uint32_t*>(bits);
                    for (std::size_t y = 0; y < font.atlas_.height(); ++y) {
                        for (std::size_t x = 0; x < font.atlas_.width(); ++x) {
                            const std::uint32_t bgra = rgba[y * font.atlas_.width() + x];
                            const std::uint8_t b = static_cast<std::uint8_t>(bgra & 0xFFu);
                            const std::uint8_t g = static_cast<std::uint8_t>((bgra >> 8u) & 0xFFu);
                            const std::uint8_t r = static_cast<std::uint8_t>((bgra >> 16u) & 0xFFu);
                            const std::uint8_t alpha = static_cast<std::uint8_t>((static_cast<unsigned int>(r) + g + b) / 3u);
                            font.atlas_.at(x, y) = Color8{255, 255, 255, alpha};
                        }
                    }

                    SelectObject(dc, oldFont);
                    DeleteObject(hfont);
                }

                SelectObject(dc, oldBitmap);
                DeleteObject(bitmap);
            }
            DeleteDC(dc);
        }
#else
        for (std::size_t i = 0; i < glyphCount; ++i) {
            const char ch = static_cast<char>(firstChar + i);
            const std::size_t col = i % columns;
            const std::size_t row = i / columns;
            const std::size_t ox = col * cellWidth + 1;
            const std::size_t oy = row * cellHeight + 1;

            auto& glyph = font.glyphs_[static_cast<std::size_t>(static_cast<unsigned char>(ch))];
            glyph = {ox, oy, cellWidth - 2, cellHeight - 2, static_cast<int>(cellWidth - 3)};

            for (std::size_t y = 0; y + 1 < cellHeight; ++y) {
                for (std::size_t x = 0; x + 1 < cellWidth; ++x) {
                    const unsigned bit = static_cast<unsigned>((x * 3 + y * 5 + static_cast<unsigned char>(ch)) & 7u);
                    const std::uint8_t a = (bit < 3u) ? 255u : 0u;
                    font.atlas_.at(ox + x, oy + y) = Color8{255, 255, 255, a};
                }
            }
        }
#endif

        return font;
    }

    const Surface<Color8>& atlas() const { return atlas_; }
    const FontGlyph* lookup(char c) const {
        const auto& g = glyphs_[static_cast<std::size_t>(static_cast<unsigned char>(c))];
        return (g.width == 0 || g.height == 0) ? nullptr : &g;
    }
    std::size_t lineHeight() const { return lineHeight_; }

private:
    Surface<Color8> atlas_{1, 1, Color8{0, 0, 0, 0}};
    std::array<FontGlyph, 256> glyphs_{};
    std::size_t lineHeight_{10};
};

template <typename PixelT>
void renderText(const la::Vec2<int>& position,
                std::string_view text,
                const Colorf& color,
                const Font& font,
                Surface<PixelT>& renderTarget) {
    int cursorX = position.x;
    int cursorY = position.y;
    for (char c : text) {
        if (c == '\n') {
            cursorX = position.x;
            cursorY += static_cast<int>(font.lineHeight());
            continue;
        }

        const auto* glyph = font.lookup(c);
        if (glyph == nullptr) {
            cursorX += static_cast<int>(font.lineHeight() / 2);
            continue;
        }

        for (std::size_t gy = 0; gy < glyph->height; ++gy) {
            const int py = cursorY + static_cast<int>(gy);
            if (py < 0 || py >= static_cast<int>(renderTarget.height())) {
                continue;
            }
            for (std::size_t gx = 0; gx < glyph->width; ++gx) {
                const int px = cursorX + static_cast<int>(gx);
                if (px < 0 || px >= static_cast<int>(renderTarget.width())) {
                    continue;
                }

                const auto mask = font.atlas().at(glyph->u + gx, glyph->v + gy).template cast<float>();
                const float srcAlpha = color.a * mask.a;
                if (srcAlpha <= 0.001f) {
                    continue;
                }

                const auto dst = renderTarget.at(static_cast<std::size_t>(px), static_cast<std::size_t>(py)).template cast<float>();
                const float outAlpha = srcAlpha + dst.a * (1.0f - srcAlpha);

                Colorf out{
                    color.r * srcAlpha + dst.r * (1.0f - srcAlpha),
                    color.g * srcAlpha + dst.g * (1.0f - srcAlpha),
                    color.b * srcAlpha + dst.b * (1.0f - srcAlpha),
                    outAlpha};

                if constexpr (!std::is_floating_point_v<typename PixelT::value_type>) {
                    out.r = std::clamp(out.r, 0.0f, 1.0f);
                    out.g = std::clamp(out.g, 0.0f, 1.0f);
                    out.b = std::clamp(out.b, 0.0f, 1.0f);
                    out.a = std::clamp(out.a, 0.0f, 1.0f);
                }

                renderTarget.at(static_cast<std::size_t>(px), static_cast<std::size_t>(py)) =
                    out.template cast<typename PixelT::value_type>();
            }
        }

        cursorX += glyph->advance;
        if (cursorX >= static_cast<int>(renderTarget.width())) {
            break;
        }
    }

}

} // namespace gfx
