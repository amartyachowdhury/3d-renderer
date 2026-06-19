#include "renderer/raycaster/map.h"

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

    const std::vector<std::string> rows = {
        "111",
        "1.1",
        "111",
    };
    Map map(rows);

    EXPECT_TRUE(map.width() == 3);
    EXPECT_TRUE(map.height() == 3);
    EXPECT_TRUE(map.tile(0, 0) == 1);
    EXPECT_TRUE(map.tile(1, 1) == 0);
    EXPECT_TRUE(map.tile(2, 1) == 1);

    SpawnData spawn;
    std::string error;
    EXPECT_TRUE(load_spawn("assets/maps/level1.spawn", spawn, error));
    EXPECT_TRUE(!spawn.sprites.empty());
    EXPECT_TRUE(spawn.player.x > 0.0);

    if (failures == 0) {
        std::cout << "All raycaster tests passed\n";
        return 0;
    }

    std::cerr << failures << " raycaster test(s) failed\n";
    return 1;
}
