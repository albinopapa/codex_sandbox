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
- Parameterized mesh generation for planes, cubes, and spheres.
- A camera with pan, rotate, and zoom controls that produces a view matrix.
- A view frustum utility to cull objects outside camera visibility.
- A templated `SoftwareRenderer<Config, PixelT>` with triangle rasterization, depth testing, and optional texturing.


## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/software_renderer_demo
```

On Windows, the demo opens a real-time display window and continuously presents frames.
On non-Windows platforms, the display class is a no-op fallback so the sample still runs and writes `render.ppm`.

## TODO: Galaga clone MVP plan

- [x] **Define a simple game loop state**
  - Add minimal states: `Title`, `Playing`, and `GameOver`.
  - Keep simulation fixed-step and rendering straightforward.

- [x] **Player ship (sphere placeholder)**
  - Represent the player as a sphere mesh near the bottom of the play area.
  - Implement left/right movement with existing keyboard input.
  - Clamp player movement to screen bounds.

- [x] **Enemy ships (cube placeholders of different sizes)**
  - Spawn enemies in a grid with at least 2–3 cube sizes to represent different ship types.
  - Start with simple movement patterns (horizontal sweep + downward step).
  - Track alive/dead state per enemy.

- [x] **Projectile system (minimal)**
  - Player fires one projectile type upward.
  - Enemies optionally fire basic downward shots after player firing is stable.
  - Use lightweight structs and capped projectile counts for MVP.

- [x] **Collision and life rules**
  - Add basic sphere/box or bounding-radius collision checks.
  - Destroy enemy on player projectile hit and increment score.
  - Reduce player lives on enemy hit or enemy projectile hit.

- [x] **Score + UI text using `Font`**
  - Render score in a screen corner (e.g., `SCORE: ####`).
  - Show simple prompts: `PRESS SPACE TO START`, `GAME OVER`, `PRESS R TO RESTART`.
  - Keep text rendering centralized through the existing font class.

- [x] **Win/lose flow**
  - Win condition: all enemies destroyed.
  - Lose condition: no lives remaining (or enemies reach player row).
  - Restart returns to `Title` or starts a fresh `Playing` session.

- [x] **MVP polish (only essentials)**
  - Distinguish enemy cube sizes with clear colors.
  - Tune movement/fire speed so one round is playable in under a minute.
  - Defer advanced Galaga behavior (dives, formations, animations, audio) until after MVP.
