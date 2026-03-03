#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>

namespace la {

template <typename T>
struct Vec2 {
    T x{};
    T y{};

    constexpr Vec2() = default;
    constexpr Vec2(T x_, T y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    constexpr Vec2 operator-(const Vec2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    constexpr Vec2 operator*(T s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(T s) const { return {x / s, y / s}; }
    constexpr Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }

    constexpr T dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }
    T length() const { return std::sqrt(dot(*this)); }
    Vec2 normalized() const { auto len = length(); return len == T{} ? Vec2{} : (*this / len); }
};

template <typename T>
struct Vec3 {
    T x{};
    T y{};
    T z{};

    constexpr Vec3() = default;
    constexpr Vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    constexpr Vec3 operator-(const Vec3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    constexpr Vec3 operator*(T s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(T s) const { return {x / s, y / s, z / s}; }
    constexpr Vec3& operator+=(const Vec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }

    constexpr T dot(const Vec3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
    constexpr Vec3 cross(const Vec3& rhs) const {
        return {y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x};
    }
    T length() const { return std::sqrt(dot(*this)); }
    Vec3 normalized() const { auto len = length(); return len == T{} ? Vec3{} : (*this / len); }
};

template <typename T>
struct Vec4 {
    T x{};
    T y{};
    T z{};
    T w{};

    constexpr Vec4() = default;
    constexpr Vec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(const Vec3<T>& v, T w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    constexpr Vec4 operator+(const Vec4& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w}; }
    constexpr Vec4 operator-(const Vec4& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w}; }
    constexpr Vec4 operator*(T s) const { return {x * s, y * s, z * s, w * s}; }
    constexpr Vec4 operator/(T s) const { return {x / s, y / s, z / s, w / s}; }

    constexpr T dot(const Vec4& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
    T length() const { return std::sqrt(dot(*this)); }
    Vec4 normalized() const { auto len = length(); return len == T{} ? Vec4{} : (*this / len); }

    constexpr Vec3<T> xyz() const { return {x, y, z}; }
};

template <typename T>
struct Mat3x2 {
    std::array<T, 6> m{}; // row-major: [r0c0 r0c1 r1c0 r1c1 r2c0 r2c1]

    constexpr Mat3x2() = default;
    constexpr Mat3x2(std::initializer_list<T> vals) {
        if (vals.size() != 6) throw std::runtime_error("Mat3x2 requires exactly 6 values");
        std::size_t i = 0;
        for (auto v : vals) m[i++] = v;
    }

    constexpr T& operator()(std::size_t r, std::size_t c) { return m[r * 2 + c]; }
    constexpr const T& operator()(std::size_t r, std::size_t c) const { return m[r * 2 + c]; }

    constexpr Vec3<T> multiply(const Vec2<T>& v) const {
        return {
            (*this)(0, 0) * v.x + (*this)(0, 1) * v.y,
            (*this)(1, 0) * v.x + (*this)(1, 1) * v.y,
            (*this)(2, 0) * v.x + (*this)(2, 1) * v.y
        };
    }
};

template <typename T>
struct Mat4 {
    std::array<T, 16> m{}; // row-major

    constexpr Mat4() = default;
    constexpr Mat4(std::initializer_list<T> vals) {
        if (vals.size() != 16) throw std::runtime_error("Mat4 requires exactly 16 values");
        std::size_t i = 0;
        for (auto v : vals) m[i++] = v;
    }

    static constexpr Mat4 identity() {
        return Mat4{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }

    constexpr T& operator()(std::size_t r, std::size_t c) { return m[r * 4 + c]; }
    constexpr const T& operator()(std::size_t r, std::size_t c) const { return m[r * 4 + c]; }

    constexpr Mat4 operator*(const Mat4& rhs) const {
        Mat4 out;
        for (std::size_t r = 0; r < 4; ++r) {
            for (std::size_t c = 0; c < 4; ++c) {
                T sum{};
                for (std::size_t k = 0; k < 4; ++k) {
                    sum += (*this)(r, k) * rhs(k, c);
                }
                out(r, c) = sum;
            }
        }
        return out;
    }

    constexpr Vec4<T> operator*(const Vec4<T>& v) const {
        return {
            (*this)(0, 0) * v.x + (*this)(0, 1) * v.y + (*this)(0, 2) * v.z + (*this)(0, 3) * v.w,
            (*this)(1, 0) * v.x + (*this)(1, 1) * v.y + (*this)(1, 2) * v.z + (*this)(1, 3) * v.w,
            (*this)(2, 0) * v.x + (*this)(2, 1) * v.y + (*this)(2, 2) * v.z + (*this)(2, 3) * v.w,
            (*this)(3, 0) * v.x + (*this)(3, 1) * v.y + (*this)(3, 2) * v.z + (*this)(3, 3) * v.w
        };
    }

    static Mat4 translation(T x, T y, T z) {
        auto out = identity();
        out(0, 3) = x;
        out(1, 3) = y;
        out(2, 3) = z;
        return out;
    }

    static Mat4 scale(T x, T y, T z) {
        Mat4 out{};
        out(0, 0) = x;
        out(1, 1) = y;
        out(2, 2) = z;
        out(3, 3) = 1;
        return out;
    }

    static Mat4 perspective(T fovYRadians, T aspect, T nearZ, T farZ) {
        const T f = static_cast<T>(1) / std::tan(fovYRadians / static_cast<T>(2));
        Mat4 out{};
        out(0, 0) = f / aspect;
        out(1, 1) = f;
        out(2, 2) = (farZ + nearZ) / (nearZ - farZ);
        out(2, 3) = (static_cast<T>(2) * farZ * nearZ) / (nearZ - farZ);
        out(3, 2) = static_cast<T>(-1);
        return out;
    }
};

} // namespace la
