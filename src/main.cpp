#include <fstream>

#include "renderer.hpp"

struct DemoConfig {
    static constexpr bool enableTexturing = true;
    static constexpr bool enableDepthTest = true;
};

int main() {
    using Pixel = gfx::Color8;
    gfx::Surface<Pixel> framebuffer(256, 256, Pixel{0, 0, 0, 255});
    gfx::Surface<Pixel> checker(2, 2, Pixel{255, 255, 255, 255});
    checker.at(1, 0) = Pixel{20, 20, 20, 255};
    checker.at(0, 1) = Pixel{20, 20, 20, 255};

    gfx::SoftwareRenderer<DemoConfig, Pixel> renderer(framebuffer);
    renderer.clear(Pixel{30, 30, 45, 255});

    using V = gfx::SoftwareRenderer<DemoConfig, Pixel>::Vertex;
    V v0{{-0.8f, -0.8f, 0.1f}, {0.0f, 1.0f}, {1.0f, 0.2f, 0.2f, 1.0f}};
    V v1{{ 0.8f, -0.8f, 0.1f}, {1.0f, 1.0f}, {0.2f, 1.0f, 0.2f, 1.0f}};
    V v2{{ 0.0f,  0.8f, 0.1f}, {0.5f, 0.0f}, {0.2f, 0.4f, 1.0f, 1.0f}};

    auto mvp = la::Mat4<float>::identity();
    renderer.drawTriangle(v0, v1, v2, mvp, &checker);

    std::ofstream ppm("render.ppm");
    ppm << "P3\n" << framebuffer.width() << ' ' << framebuffer.height() << "\n255\n";
    for (std::size_t y = 0; y < framebuffer.height(); ++y) {
        for (std::size_t x = 0; x < framebuffer.width(); ++x) {
            const auto px = framebuffer.at(x, y);
            ppm << static_cast<int>(px.r) << ' ' << static_cast<int>(px.g) << ' ' << static_cast<int>(px.b) << '\n';
        }
    }

    return 0;
}
