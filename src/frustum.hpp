#pragma once

#include <array>
#include <cmath>

#include "math.hpp"

namespace gfx {

class Frustum {
public:
    struct Plane {
        la::Vec3<float> normal;
        float d;

        float distanceTo(const la::Vec3<float>& p) const {
            return normal.dot(p) + d;
        }

        void normalize() {
            const float invLen = 1.0f / normal.length();
            normal = normal * invLen;
            d *= invLen;
        }
    };

    static Frustum fromViewProjection(const la::Mat4<float>& m) {
        Frustum f;

        f.planes_[0] = {{m(3,0) + m(0,0), m(3,1) + m(0,1), m(3,2) + m(0,2)}, m(3,3) + m(0,3)}; // left
        f.planes_[1] = {{m(3,0) - m(0,0), m(3,1) - m(0,1), m(3,2) - m(0,2)}, m(3,3) - m(0,3)}; // right
        f.planes_[2] = {{m(3,0) + m(1,0), m(3,1) + m(1,1), m(3,2) + m(1,2)}, m(3,3) + m(1,3)}; // bottom
        f.planes_[3] = {{m(3,0) - m(1,0), m(3,1) - m(1,1), m(3,2) - m(1,2)}, m(3,3) - m(1,3)}; // top
        f.planes_[4] = {{m(3,0) + m(2,0), m(3,1) + m(2,1), m(3,2) + m(2,2)}, m(3,3) + m(2,3)}; // near
        f.planes_[5] = {{m(3,0) - m(2,0), m(3,1) - m(2,1), m(3,2) - m(2,2)}, m(3,3) - m(2,3)}; // far

        for (auto& plane : f.planes_) {
            plane.normalize();
        }

        return f;
    }

    bool containsSphere(const la::Vec3<float>& center, float radius) const {
        for (const auto& plane : planes_) {
            if (plane.distanceTo(center) < -radius) {
                return false;
            }
        }
        return true;
    }

    bool containsAabb(const la::Vec3<float>& minCorner, const la::Vec3<float>& maxCorner) const {
        for (const auto& plane : planes_) {
            la::Vec3<float> p = minCorner;
            if (plane.normal.x >= 0.0f) p.x = maxCorner.x;
            if (plane.normal.y >= 0.0f) p.y = maxCorner.y;
            if (plane.normal.z >= 0.0f) p.z = maxCorner.z;
            if (plane.distanceTo(p) < 0.0f) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<Plane, 6> planes_{};
};

} // namespace gfx
