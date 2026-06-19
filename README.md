# Build Your Own 3D Renderer

A C++ learning project that implements three classic rendering approaches from scratch—no GPU APIs, only software rendering on the CPU.

## Phases

| Phase | Target | Description |
|-------|--------|-------------|
| 1 | `raytracer` | Recursive ray tracing with materials (RTOW-style) |
| 2 | `rasterizer` | Triangle mesh rasterization with z-buffer |
| 3 | `raycaster` | Wolfenstein-style 2.5D grid raycasting |

## Coordinate system

- **Handedness:** Right-handed
- **Up axis:** +Y
- **Camera forward:** Looks down **-Z** in view space (standard OpenGL-style camera math)
- **Color:** Linear RGB internally; gamma 2.0 applied on output

## Dependencies

macOS (Homebrew):

```bash
brew install cmake sdl2
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

### Phase 1 — Ray tracer

Random sphere scene (default):

```bash
./build/raytracer --output image.ppm
```

Cornell-style scene file with live SDL preview:

```bash
./build/raytracer --scene assets/scenes/cornell.scene --preview --samples 40
```

Options: `--width`, `--height`, `--samples`, `--max-depth`, `--threads`, `--obj`, `--camera-yaw`, `--dump-ppm`

### Phase 2 — Software rasterizer

Rotating cube (default):

```bash
./build/rasterizer
```

Load an OBJ mesh:

```bash
./build/rasterizer --obj assets/models/cube.obj
./build/rasterizer --obj assets/models/cube.obj --texture assets/textures/checker.ppm
```

Options: `--texture`, `--output`, `--dump-ppm`

### Phase 3 — Raycaster

WASD to move, Left/Right arrows to turn. Minimap shown in the top-left.

```bash
./build/raycaster
./build/raycaster --map assets/maps/level1.txt
```

Options: `--map`, `--textures`, `--output`, `--dump-ppm`

## Testing

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Smoke-test all renderers headlessly:

```bash
./build/raytracer --width 64 --height 36 --samples 2 --dump-ppm
./build/rasterizer --dump-ppm
./build/raycaster --dump-ppm
```

## Project layout

```
include/renderer/   Shared headers (math, core, platform)
src/raytracer/      Phase 1
src/rasterizer/     Phase 2
src/raycaster/      Phase 3
assets/             Scenes, models, textures
```

## References

Inspired by [build-your-own-x](https://github.com/codecrafters-io/build-your-own-x#build-your-own-3d-renderer):

- [Ray Tracing in One Weekend](https://raytracing.github.io/)
- [Rasterization: a Practical Implementation](https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation)
- [Raycasting engine of Wolfenstein 3D](https://lodev.org/cgtutor/raycasting.html)

## License

This project is licensed under the [MIT License](LICENSE).
