#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "math.hpp"
#include "surface.hpp"

namespace gfx {

struct DefaultRendererConfig {
    static constexpr bool enableTexturing = true;
    static constexpr bool enableDepthTest = true;
};

template <typename Config, typename PixelT = Colorf>
class SoftwareRenderer {
public:
    using Vec2f = la::Vec2<float>;
    using Vec3f = la::Vec3<float>;
    using Vec4f = la::Vec4<float>;
    using Mat4f = la::Mat4<float>;

    struct Vertex {
        Vec3f position;
        Vec2f uv;
        Colorf color;
    };

    explicit SoftwareRenderer(Surface<PixelT>& target)
        : target_(target), depthBuffer_(target.width() * target.height(), std::numeric_limits<float>::infinity()) {}

    void clear(const PixelT& color, float depth = std::numeric_limits<float>::infinity()) {
        target_.clear(color);
        std::fill(depthBuffer_.begin(), depthBuffer_.end(), depth);
    }

    void drawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2,
                      const Mat4f& mvp,
                      const Surface<PixelT>* texture = nullptr) {
        auto p0 = transform(v0.position, mvp);
        auto p1 = transform(v1.position, mvp);
        auto p2 = transform(v2.position, mvp);

        const int minX = std::max(0, static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x}))));
        const int maxX = std::min(static_cast<int>(target_.width() - 1), static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x}))));
        const int minY = std::max(0, static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y}))));
        const int maxY = std::min(static_cast<int>(target_.height() - 1), static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y}))));

        const float area = edgeFunction(p0, p1, p2);
        if (std::abs(area) < 1e-6f) {
            return;
        }

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                Vec3f p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, 0.0f};
                float w0 = edgeFunction(p1, p2, p);
                float w1 = edgeFunction(p2, p0, p);
                float w2 = edgeFunction(p0, p1, p);

                const bool isInside = (w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0);
                if (!isInside) continue;

                w0 /= area;
                w1 /= area;
                w2 /= area;

                const float depth = p0.z * w0 + p1.z * w1 + p2.z * w2;
                if constexpr (Config::enableDepthTest) {
                    if (!depthTest(static_cast<std::size_t>(x), static_cast<std::size_t>(y), depth)) {
                        continue;
                    }
                }

                Colorf color = {
                    v0.color.r * w0 + v1.color.r * w1 + v2.color.r * w2,
                    v0.color.g * w0 + v1.color.g * w1 + v2.color.g * w2,
                    v0.color.b * w0 + v1.color.b * w1 + v2.color.b * w2,
                    v0.color.a * w0 + v1.color.a * w1 + v2.color.a * w2,
                };

                if constexpr (Config::enableTexturing) {
                    if (texture != nullptr) {
                        const Vec2f uv = v0.uv * w0 + v1.uv * w1 + v2.uv * w2;
                        const auto texel = texture->sampleNearest(uv.x, uv.y).template cast<float>();
                        color = {
                            color.r * texel.r,
                            color.g * texel.g,
                            color.b * texel.b,
                            color.a * texel.a
                        };
                    }
                }

                target_.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y)) = color.template cast<typename PixelT::value_type>();
            }
        }
    }

private:
    struct ScreenPoint {
        float x;
        float y;
        float z;
    };

    ScreenPoint transform(const Vec3f& position, const Mat4f& mvp) const {
        Vec4f clip = mvp * Vec4f(position, 1.0f);
        const float invW = 1.0f / clip.w;
        const float ndcX = clip.x * invW;
        const float ndcY = clip.y * invW;
        const float ndcZ = clip.z * invW;

        return {
            (ndcX * 0.5f + 0.5f) * static_cast<float>(target_.width() - 1),
            (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(target_.height() - 1),
            ndcZ
        };
    }

    static float edgeFunction(const ScreenPoint& a, const ScreenPoint& b, const Vec3f& c) {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    static float edgeFunction(const ScreenPoint& a, const ScreenPoint& b, const ScreenPoint& c) {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    bool depthTest(std::size_t x, std::size_t y, float depth) {
        const std::size_t idx = y * target_.width() + x;
        if (depth >= depthBuffer_[idx]) return false;
        depthBuffer_[idx] = depth;
        return true;
    }

    Surface<PixelT>& target_;
    std::vector<float> depthBuffer_;
};

} // namespace gfx
