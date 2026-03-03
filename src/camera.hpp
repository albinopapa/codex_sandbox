#pragma once

#include <algorithm>
#include <cmath>

#include "math.hpp"

namespace gfx {

class Camera {
public:
    Camera(la::Vec3<float> position = {0.0f, 0.0f, 3.0f}, float yawRadians = 0.0f, float pitchRadians = 0.0f)
        : position_(position), yaw_(yawRadians), pitch_(pitchRadians) {}

    void pan(float dx, float dy) {
        const auto right = rightVector();
        const auto up = upVector();
        position_ += right * dx + up * dy;
    }

    void rotate(float deltaYawRadians, float deltaPitchRadians) {
        yaw_ += deltaYawRadians;
        pitch_ += deltaPitchRadians;
        const float halfPi = 1.57079632679f;
        pitch_ = std::clamp(pitch_, -halfPi + 0.001f, halfPi - 0.001f);
    }

    void zoom(float amount) {
        position_ += forwardVector() * amount;
    }

    la::Mat4<float> viewMatrix() const {
        const la::Vec3<float> f = forwardVector().normalized();
        const la::Vec3<float> upWorld{0.0f, 1.0f, 0.0f};
        const la::Vec3<float> r = f.cross(upWorld).normalized();
        const la::Vec3<float> u = r.cross(f);

        return la::Mat4<float>{
             r.x,  r.y,  r.z, -r.dot(position_),
             u.x,  u.y,  u.z, -u.dot(position_),
            -f.x, -f.y, -f.z,  f.dot(position_),
             0.0f, 0.0f, 0.0f, 1.0f
        };
    }

    const la::Vec3<float>& position() const { return position_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }

private:
    la::Vec3<float> forwardVector() const {
        const float cp = std::cos(pitch_);
        return la::Vec3<float>{
            std::sin(yaw_) * cp,
            std::sin(pitch_),
            -std::cos(yaw_) * cp
        }.normalized();
    }

    la::Vec3<float> rightVector() const {
        return forwardVector().cross({0.0f, 1.0f, 0.0f}).normalized();
    }

    la::Vec3<float> upVector() const {
        return rightVector().cross(forwardVector()).normalized();
    }

    la::Vec3<float> position_;
    float yaw_;
    float pitch_;
};

} // namespace gfx
