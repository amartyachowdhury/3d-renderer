#include "renderer/math/mat4.h"
#include "renderer/math/vec3.h"

#include <cassert>
#include <cmath>
#include <iostream>

#define EXPECT_TRUE(expr)                                                                 \
    do {                                                                                  \
        if (!(expr)) {                                                                    \
            std::cerr << "FAIL: " << __FILE__ << ':' << __LINE__ << " " #expr << '\n';    \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

#define EXPECT_NEAR(a, b, eps) EXPECT_TRUE(std::fabs((a) - (b)) <= (eps))

int failures = 0;

int main() {
    using namespace renderer;

    Vec3 a{1.0, 2.0, 3.0};
    Vec3 b{4.0, 5.0, 6.0};
    EXPECT_NEAR(a.dot(b), 32.0, 1e-9);
    EXPECT_NEAR(a.cross(b).x, -3.0, 1e-9);
    EXPECT_NEAR(a.cross(b).y, 6.0, 1e-9);
    EXPECT_NEAR(a.cross(b).z, -3.0, 1e-9);
    EXPECT_NEAR(a.normalized().length(), 1.0, 1e-9);

    Vec3 reflected = Vec3::reflect(Vec3{1.0, -1.0, 0.0}, Vec3{0.0, 1.0, 0.0});
    EXPECT_NEAR(reflected.y, 1.0, 1e-9);

    Mat4 identity = Mat4::identity();
    Vec4 transformed = identity.transform_point(Vec4{2.0, 3.0, 4.0, 1.0});
    EXPECT_NEAR(transformed.x, 2.0, 1e-9);
    EXPECT_NEAR(transformed.y, 3.0, 1e-9);
    EXPECT_NEAR(transformed.z, 4.0, 1e-9);

    Mat4 translation = Mat4::translation(Vec3{1.0, 2.0, 3.0});
    transformed = translation.transform_point(Vec4{0.0, 0.0, 0.0, 1.0});
    EXPECT_NEAR(transformed.x, 1.0, 1e-9);
    EXPECT_NEAR(transformed.y, 2.0, 1e-9);
    EXPECT_NEAR(transformed.z, 3.0, 1e-9);

    if (failures == 0) {
        std::cout << "All math tests passed\n";
        return 0;
    }

    std::cerr << failures << " math test(s) failed\n";
    return 1;
}
