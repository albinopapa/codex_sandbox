#include <chrono>
#include <cmath>
#include <fstream>
#include <thread>
#include <vector>

#include "camera.hpp"
#include "display.hpp"
#include "frustum.hpp"
#include "geometry.hpp"
#include <vector>

#include "camera.hpp"
#include "frustum.hpp"
#include "geometry.hpp"

#include "renderer.hpp"

struct DemoConfig {
    static constexpr bool enableTexturing = true;
    static constexpr bool enableDepthTest = true;
    static constexpr bool enableBackfaceCulling = true;
};

struct RenderObject {
    gfx::Mesh mesh;
    gfx::Colorf tint;
    la::Vec3<float> translation;
    float boundingRadius;
};

int main() {
    using Pixel = gfx::Color8;
    constexpr std::size_t width = 512;
    constexpr std::size_t height = 512;

    gfx::Surface<Pixel> framebuffer(width, height, Pixel{0, 0, 0, 255});
    gfx::Surface<Pixel> checker(4, 4, Pixel{255, 255, 255, 255});
    for (std::size_t y = 0; y < checker.height(); ++y) {
        for (std::size_t x = 0; x < checker.width(); ++x) {
            if ((x + y) % 2 == 1) {
                checker.at(x, y) = Pixel{40, 40, 40, 255};
            }
        }
    }

    gfx::SoftwareRenderer<DemoConfig, Pixel> renderer(framebuffer);
    gfx::Display<Pixel> display(width, height, "Software Renderer");

    gfx::Camera camera({0.0f, 0.5f, 4.0f});

    std::vector<RenderObject> objects;
    objects.push_back({gfx::makeCube(1.1f, 1.1f, 1.1f), {1.0f, 0.6f, 0.4f, 1.0f}, {-1.2f, 0.6f, -2.2f}, 1.0f});
    objects.push_back({gfx::makeSphere(0.65f, 30, 20), {0.4f, 0.8f, 1.0f, 1.0f}, {1.3f, 0.7f, -3.4f}, 0.7f});
    objects.push_back({gfx::makePlane(5.0f, 5.0f, 8, 8), {0.8f, 0.85f, 0.8f, 1.0f}, {0.0f, -0.1f, -3.0f}, 3.8f});
    objects.push_back({gfx::makeCube(1.0f, 1.0f, 1.0f), {1.0f, 0.2f, 0.8f, 1.0f}, {30.0f, 0.0f, -10.0f}, 1.0f}); // culled

    using RV = gfx::SoftwareRenderer<DemoConfig, Pixel>::Vertex;

    std::size_t frame = 0;
#ifndef _WIN32
    constexpr std::size_t maxFrames = 120;
#endif

    while (display.processEvents()) {
#ifndef _WIN32
        if (frame++ >= maxFrames) {
            break;
        }
#endif

        const float t = static_cast<float>(frame) * 0.02f;
        camera.rotate(0.003f, 0.0f);
        camera.pan(0.0005f * std::sin(t), 0.0f);

        renderer.clear(Pixel{26, 30, 44, 255});

        const auto proj = la::Mat4<float>::perspective(1.0472f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 50.0f);
        const auto view = camera.viewMatrix();
        const auto viewProj = proj * view;
        const auto frustum = gfx::Frustum::fromViewProjection(viewProj);

        for (const auto& object : objects) {
            if (!frustum.containsSphere(object.translation, object.boundingRadius)) {
                continue;
            }

            const auto model = la::Mat4<float>::translation(object.translation.x, object.translation.y, object.translation.z);
            const auto mvp = viewProj * model;

            for (std::size_t i = 0; i + 2 < object.mesh.indices.size(); i += 3) {
                const auto& a = object.mesh.vertices[object.mesh.indices[i]];
                const auto& b = object.mesh.vertices[object.mesh.indices[i + 1]];
                const auto& c = object.mesh.vertices[object.mesh.indices[i + 2]];

                RV v0{a.position, a.uv, object.tint};
                RV v1{b.position, b.uv, object.tint};
                RV v2{c.position, c.uv, object.tint};
                renderer.drawTriangle(v0, v1, v2, mvp, &checker);
            }
        }

        display.present(framebuffer);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

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
