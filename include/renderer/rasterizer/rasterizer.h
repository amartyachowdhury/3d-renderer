#pragma once

#include "renderer/core/framebuffer.h"
#include "renderer/math/mat4.h"
#include "renderer/rasterizer/mesh.h"

namespace renderer {

struct Light {
    Vec3 direction{-0.5, -1.0, -0.3};
    Color color{1.0, 1.0, 1.0};
    Color ambient{0.15, 0.15, 0.18};
};

class SoftwareRasterizer {
public:
    SoftwareRasterizer(int width, int height);

    void clear(const Color& color);
    void draw_mesh(const Mesh& mesh, const Mat4& model, const Mat4& view, const Mat4& projection, const Light& light, const Texture* texture = nullptr);
    void present(Framebuffer& framebuffer) const;

private:
    int width_;
    int height_;
    std::vector<Color> color_buffer_;
    std::vector<double> depth_buffer_;

    void set_pixel(int x, int y, double depth, const Color& color);
    void draw_triangle(
        const Vertex& v0,
        const Vertex& v1,
        const Vertex& v2,
        const Mat4& mvp,
        const Light& light,
        const Texture* texture);
};

}  // namespace renderer
