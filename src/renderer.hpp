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
    static constexpr bool enableBackfaceCulling = true;
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
        std::vector<ClipVertex> clipped = {
            makeClipVertex(v0, mvp),
            makeClipVertex(v1, mvp),
            makeClipVertex(v2, mvp)
        };

        clipToViewFrustum(clipped);
        if (clipped.size() < 3) {
            return;
        }

        const ClipVertex& base = clipped[0];
        for (std::size_t i = 1; i + 1 < clipped.size(); ++i) {
            rasterizeTriangle(base, clipped[i], clipped[i + 1], texture);
        }
    }

private:
    struct ScreenPoint {
        float x;
        float y;
        float z;
    };

    struct ClipVertex {
        Vec4f clip;
        Vec2f uv;
        Colorf color;
    };

    static ClipVertex interpolate(const ClipVertex& a, const ClipVertex& b, float t) {
        return {
            a.clip + (b.clip - a.clip) * t,
            a.uv + (b.uv - a.uv) * t,
            {
                a.color.r + (b.color.r - a.color.r) * t,
                a.color.g + (b.color.g - a.color.g) * t,
                a.color.b + (b.color.b - a.color.b) * t,
                a.color.a + (b.color.a - a.color.a) * t
            }
        };
    }

    template <typename DistFn>
    static void clipAgainstPlane(std::vector<ClipVertex>& polygon, DistFn distanceFn) {
        if (polygon.empty()) {
            return;
        }

        std::vector<ClipVertex> output;
        output.reserve(polygon.size() + 1);

        ClipVertex prev = polygon.back();
        float prevDistance = distanceFn(prev);
        bool prevInside = prevDistance >= 0.0f;

        for (const auto& current : polygon) {
            const float currentDistance = distanceFn(current);
            const bool currentInside = currentDistance >= 0.0f;

            if (currentInside != prevInside) {
                const float t = prevDistance / (prevDistance - currentDistance);
                output.push_back(interpolate(prev, current, t));
            }

            if (currentInside) {
                output.push_back(current);
            }

            prev = current;
            prevDistance = currentDistance;
            prevInside = currentInside;
        }

        polygon.swap(output);
    }

    static void clipToViewFrustum(std::vector<ClipVertex>& polygon) {
        clipAgainstPlane(polygon, [](const ClipVertex& v) { return v.clip.x + v.clip.w; });
        clipAgainstPlane(polygon, [](const ClipVertex& v) { return v.clip.w - v.clip.x; });
        clipAgainstPlane(polygon, [](const ClipVertex& v) { return v.clip.y + v.clip.w; });
        clipAgainstPlane(polygon, [](const ClipVertex& v) { return v.clip.w - v.clip.y; });
        clipAgainstPlane(polygon, [](const ClipVertex& v) { return v.clip.z + v.clip.w; });
        clipAgainstPlane(polygon, [](const ClipVertex& v) { return v.clip.w - v.clip.z; });
    }

    ClipVertex makeClipVertex(const Vertex& vertex, const Mat4f& mvp) const {
        return {
            mvp * Vec4f(vertex.position, 1.0f),
            vertex.uv,
            vertex.color
        };
    }

    ScreenPoint toScreen(const Vec4f& clip) const {
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

    void rasterizeTriangle(const ClipVertex& v0, const ClipVertex& v1, const ClipVertex& v2,
                           const Surface<PixelT>* texture) {
        auto p0 = toScreen(v0.clip);
        auto p1 = toScreen(v1.clip);
        auto p2 = toScreen(v2.clip);

        const float area = edgeFunction(p0, p1, p2);
        if (std::abs(area) < 1e-6f) {
            return;
        }

        if constexpr (Config::enableBackfaceCulling) {
            if (area >= 0.0f) {
                return;
            }
        }

        const int minX = std::max(0, static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x}))));
        const int maxX = std::min(static_cast<int>(target_.width() - 1), static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x}))));
        const int minY = std::max(0, static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y}))));
        const int maxY = std::min(static_cast<int>(target_.height() - 1), static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y}))));

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
