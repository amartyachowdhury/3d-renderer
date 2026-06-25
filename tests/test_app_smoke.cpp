#include "renderer/app/renderer_controller.h"
#include "renderer/core/asset_locator.h"
#include "renderer/core/framebuffer.h"

#include <iostream>
#include <string>

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

    AssetLocator assets;

    for (RendererMode mode : {RendererMode::Raytracer, RendererMode::Rasterizer, RendererMode::Raycaster}) {
        auto controller = make_controller(mode);
        EXPECT_TRUE(controller != nullptr);

        std::string error;
        EXPECT_TRUE(controller->init(assets, error));
        if (!error.empty()) {
            std::cerr << "Init warning for mode: " << error << '\n';
        }

        Framebuffer framebuffer(controller->width(), controller->height());
        controller->on_enter();
        controller->render(framebuffer);
        EXPECT_TRUE(framebuffer.width() == controller->width());
        EXPECT_TRUE(framebuffer.height() == controller->height());
    }

    if (failures == 0) {
        std::cout << "All app smoke tests passed\n";
        return 0;
    }

    std::cerr << failures << " app smoke test(s) failed\n";
    return 1;
}
