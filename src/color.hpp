#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace gfx {

template <typename T>
struct Color {
    using value_type = T;
    T r{};
    T g{};
    T b{};
    T a{channelMax()};

    constexpr Color() = default;
    constexpr Color(T r_, T g_, T b_, T a_ = channelMax()) : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr T channelMax() {
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(1);
        } else {
            return std::numeric_limits<T>::max();
        }
    }

    template <typename U>
    Color<U> cast() const {
        const auto convert = [](T v) {
            if constexpr (std::is_floating_point_v<T> && std::is_integral_v<U>) {
                const T clamped = std::clamp(v, static_cast<T>(0), static_cast<T>(1));
                return static_cast<U>(std::round(clamped * static_cast<T>(std::numeric_limits<U>::max())));
            } else if constexpr (std::is_integral_v<T> && std::is_floating_point_v<U>) {
                return static_cast<U>(v) / static_cast<U>(std::numeric_limits<T>::max());
            } else {
                return static_cast<U>(v);
            }
        };
        return {convert(r), convert(g), convert(b), convert(a)};
    }

    template <typename U>
    static Color lerp(const Color& c0, const Color& c1, U t) {
        return {
            static_cast<T>(c0.r + (c1.r - c0.r) * t),
            static_cast<T>(c0.g + (c1.g - c0.g) * t),
            static_cast<T>(c0.b + (c1.b - c0.b) * t),
            static_cast<T>(c0.a + (c1.a - c0.a) * t)
        };
    }
};

using Color8 = Color<std::uint8_t>;
using Colorf = Color<float>;

} // namespace gfx
