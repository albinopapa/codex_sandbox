# codex_sandbox

A compact C++ software rendering playground that includes:

- A linear algebra library with `Vec2`, `Vec3`, `Vec4`, `Mat3x2`, and `Mat4` types.
- A generic `Color<T>` representation supporting integer and floating-point channel storage.
- A CPU `Surface<PixelT>` image class for read/write and nearest-neighbor texture sampling.
- A templated `SoftwareRenderer<Config, PixelT>` with triangle rasterization, depth testing, and optional texturing.

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/software_renderer_demo
```

This writes a `render.ppm` image file in the project root.
