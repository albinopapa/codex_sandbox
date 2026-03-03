#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "math.hpp"

namespace gfx {

struct Mesh {
    struct Vertex {
        la::Vec3<float> position;
        la::Vec3<float> normal;
        la::Vec2<float> uv;
    };

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

inline Mesh makePlane(float width = 1.0f, float depth = 1.0f,
                      std::size_t xSegments = 1, std::size_t zSegments = 1) {
    Mesh mesh;
    xSegments = xSegments == 0 ? 1 : xSegments;
    zSegments = zSegments == 0 ? 1 : zSegments;

    const float halfW = width * 0.5f;
    const float halfD = depth * 0.5f;

    mesh.vertices.reserve((xSegments + 1) * (zSegments + 1));
    for (std::size_t z = 0; z <= zSegments; ++z) {
        for (std::size_t x = 0; x <= xSegments; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(xSegments);
            const float v = static_cast<float>(z) / static_cast<float>(zSegments);
            mesh.vertices.push_back({
                {u * width - halfW, 0.0f, v * depth - halfD},
                {0.0f, 1.0f, 0.0f},
                {u, v}
            });
        }
    }

    for (std::size_t z = 0; z < zSegments; ++z) {
        for (std::size_t x = 0; x < xSegments; ++x) {
            const std::uint32_t row0 = static_cast<std::uint32_t>(z * (xSegments + 1));
            const std::uint32_t row1 = static_cast<std::uint32_t>((z + 1) * (xSegments + 1));
            const std::uint32_t i0 = row0 + static_cast<std::uint32_t>(x);
            const std::uint32_t i1 = i0 + 1;
            const std::uint32_t i2 = row1 + static_cast<std::uint32_t>(x);
            const std::uint32_t i3 = i2 + 1;
            mesh.indices.insert(mesh.indices.end(), {i0, i2, i1, i1, i2, i3});
        }
    }

    return mesh;
}

inline Mesh makeCube(float width = 1.0f, float height = 1.0f, float depth = 1.0f) {
    Mesh mesh;
    const float hx = width * 0.5f;
    const float hy = height * 0.5f;
    const float hz = depth * 0.5f;

    const la::Vec3<float> positions[] = {
        {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz},
        { hx, -hy, -hz}, {-hx, -hy, -hz}, {-hx,  hy, -hz}, { hx,  hy, -hz},
        {-hx, -hy, -hz}, {-hx, -hy,  hz}, {-hx,  hy,  hz}, {-hx,  hy, -hz},
        { hx, -hy,  hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, { hx,  hy,  hz},
        {-hx,  hy,  hz}, { hx,  hy,  hz}, { hx,  hy, -hz}, {-hx,  hy, -hz},
        {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx, -hy,  hz}, {-hx, -hy,  hz},
    };

    const la::Vec3<float> normals[] = {
        {0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}
    };

    const la::Vec2<float> uvs[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    mesh.vertices.reserve(24);
    for (std::size_t face = 0; face < 6; ++face) {
        for (std::size_t i = 0; i < 4; ++i) {
            mesh.vertices.push_back({positions[face * 4 + i], normals[face], uvs[i]});
        }
        const std::uint32_t base = static_cast<std::uint32_t>(face * 4);
        mesh.indices.insert(mesh.indices.end(), {base + 0, base + 1, base + 2, base + 0, base + 2, base + 3});
    }

    return mesh;
}

inline Mesh makeSphere(float radius = 1.0f, std::size_t slices = 24, std::size_t stacks = 16) {
    Mesh mesh;
    slices = slices < 3 ? 3 : slices;
    stacks = stacks < 2 ? 2 : stacks;

    mesh.vertices.reserve((slices + 1) * (stacks + 1));
    for (std::size_t stack = 0; stack <= stacks; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * 3.14159265358979323846f;
        const float y = std::cos(phi);
        const float r = std::sin(phi);

        for (std::size_t slice = 0; slice <= slices; ++slice) {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * 2.0f * 3.14159265358979323846f;
            const float x = r * std::cos(theta);
            const float z = r * std::sin(theta);

            la::Vec3<float> normal{x, y, z};
            mesh.vertices.push_back({normal * radius, normal.normalized(), {u, v}});
        }
    }

    for (std::size_t stack = 0; stack < stacks; ++stack) {
        for (std::size_t slice = 0; slice < slices; ++slice) {
            const std::uint32_t row0 = static_cast<std::uint32_t>(stack * (slices + 1));
            const std::uint32_t row1 = static_cast<std::uint32_t>((stack + 1) * (slices + 1));
            const std::uint32_t i0 = row0 + static_cast<std::uint32_t>(slice);
            const std::uint32_t i1 = i0 + 1;
            const std::uint32_t i2 = row1 + static_cast<std::uint32_t>(slice);
            const std::uint32_t i3 = i2 + 1;
            mesh.indices.insert(mesh.indices.end(), {i0, i2, i1, i1, i2, i3});
        }
    }

    return mesh;
}

} // namespace gfx
