#include "renderer/core/asset_locator.h"

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

    AssetLocator locator;

    EXPECT_TRUE(locator.exists("scenes/cornell.scene"));
    EXPECT_TRUE(locator.exists("models/cube.obj"));
    EXPECT_TRUE(locator.exists("maps/level1.txt"));
    EXPECT_TRUE(locator.exists("textures/checker.png"));

    const std::string scene = locator.resolve("scenes/cornell.scene");
    EXPECT_TRUE(!scene.empty());
    EXPECT_TRUE(locator.exists(scene));

    const std::string missing = locator.resolve("does/not/exist.txt");
    EXPECT_TRUE(missing == "does/not/exist.txt");

    const std::string output = locator.user_output_path("test.png");
    EXPECT_TRUE(!output.empty());

    if (failures == 0) {
        std::cout << "All asset locator tests passed\n";
        return 0;
    }

    std::cerr << failures << " asset locator test(s) failed\n";
    return 1;
}
