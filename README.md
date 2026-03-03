# codex_sandbox

A compact C++ software rendering playground that includes:

- A linear algebra library with `Vec2`, `Vec3`, `Vec4`, `Mat3x2`, and `Mat4` types.
- A generic `Color<T>` representation supporting integer and floating-point channel storage.
- A CPU `Surface<PixelT>` image class for read/write and nearest-neighbor texture sampling.
- Parameterized mesh generation for planes, cubes, and spheres.
- A camera with pan, rotate, and zoom controls that produces a view matrix.
- A view frustum utility to cull objects outside camera visibility.
- A templated `SoftwareRenderer<Config, PixelT>` with triangle rasterization, depth testing, and optional texturing.
- A `Display<PixelT>` abstraction that presents the software-rendered surface each frame (Direct3D 11 full-screen quad on Windows).

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/software_renderer_demo
```

On Windows, the demo opens a real-time display window and continuously presents frames.
On non-Windows platforms, the display class is a no-op fallback so the sample still runs and writes `render.ppm`.
