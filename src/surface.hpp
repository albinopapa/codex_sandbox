#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "color.hpp"

namespace gfx {

template <typename PixelT>
class Surface {
public:
    Surface(std::size_t width, std::size_t height, PixelT clearColor = PixelT{})
        : width_(width), height_(height), pixels_(width * height, clearColor) {}

    std::size_t width() const { return width_; }
    std::size_t height() const { return height_; }

    PixelT& at(std::size_t x, std::size_t y) {
        boundsCheck(x, y);
        return pixels_[y * width_ + x];
    }

    const PixelT& at(std::size_t x, std::size_t y) const {
        boundsCheck(x, y);
        return pixels_[y * width_ + x];
    }

    void clear(const PixelT& c) { std::fill(pixels_.begin(), pixels_.end(), c); }

    const std::vector<PixelT>& data() const { return pixels_; }

    template <typename UV>
    PixelT sampleNearest(UV u, UV v) const {
        const auto sx = static_cast<std::size_t>(std::clamp<UV>(u, static_cast<UV>(0), static_cast<UV>(1)) * static_cast<UV>(width_ - 1));
        const auto sy = static_cast<std::size_t>(std::clamp<UV>(v, static_cast<UV>(0), static_cast<UV>(1)) * static_cast<UV>(height_ - 1));
        return at(sx, sy);
    }

private:
    void boundsCheck(std::size_t x, std::size_t y) const {
        if (x >= width_ || y >= height_) {
            throw std::out_of_range("Surface index out of range");
        }
    }

    std::size_t width_{};
    std::size_t height_{};
    std::vector<PixelT> pixels_;
};

} // namespace gfx
