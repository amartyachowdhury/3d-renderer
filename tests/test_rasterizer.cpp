#include "renderer/rasterizer/mesh.h"
#include "renderer/rasterizer/rasterizer.h"

#include <iostream>

#define EXPECT_TRUE(expr)                                                                 \
    do {                                                                                  \
        if (!(expr)) {                                                                    \
            std::cerr << "FAIL: " << __FILE__ << ':' << __LINE__ << " " #expr << '\n';    \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

int failures = 0;

int main() {
    using namespace renderer;

    SoftwareRasterizer rasterizer(64, 64);
    rasterizer.set_debug_mode(DebugMode::Depth);
    rasterizer.clear({0.0, 0.0, 0.0});

    Mesh mesh = make_cube();
    const Mat4 model = Mat4::identity();
    const Mat4 view = Mat4::look_at({0.0, 0.0, 4.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    const Mat4 projection = Mat4::perspective(60.0 * 3.14159265358979323846 / 180.0, 1.0, 0.1, 100.0);

    rasterizer.draw_mesh(mesh, model, view, projection, Light{}, nullptr, {0.0, 0.0, 4.0});

    Framebuffer framebuffer(64, 64);
    rasterizer.present(framebuffer);

    bool found_lit_pixel = false;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            const int index = (y * 64 + x) * 3;
            if (framebuffer.pixels()[index] > 0) {
                found_lit_pixel = true;
                break;
            }
        }
        if (found_lit_pixel) {
            break;
        }
    }

    EXPECT_TRUE(found_lit_pixel);

    if (failures == 0) {
        std::cout << "All rasterizer tests passed\n";
        return 0;
    }

    std::cerr << failures << " rasterizer test(s) failed\n";
    return 1;
}
