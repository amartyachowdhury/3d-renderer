#include "renderer/rasterizer/rasterizer.h"

#include "renderer/core/texture.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace renderer {

namespace {

double edge_function(const Vec3& a, const Vec3& b, const Vec3& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

}  // namespace

SoftwareRasterizer::SoftwareRasterizer(int width, int height)
    : width_(width), height_(height), color_buffer_(width * height), depth_buffer_(width * height) {
    clear({0.05, 0.07, 0.12});
}

void SoftwareRasterizer::clear(const Color& color) {
    std::fill(color_buffer_.begin(), color_buffer_.end(), color);
    std::fill(depth_buffer_.begin(), depth_buffer_.end(), std::numeric_limits<double>::infinity());
}

void SoftwareRasterizer::set_pixel(int x, int y, double depth, const Color& color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }

    const int index = y * width_ + x;
    if (depth < depth_buffer_[index]) {
        depth_buffer_[index] = depth;
        color_buffer_[index] = color;
    }
}

void SoftwareRasterizer::draw_triangle(
    const Vertex& v0,
    const Vertex& v1,
    const Vertex& v2,
    const Mat4& mvp,
    const Light& light,
    const Texture* texture) {
    Vec4 clip0 = mvp.transform_point(Vec4(v0.position, 1.0));
    Vec4 clip1 = mvp.transform_point(Vec4(v1.position, 1.0));
    Vec4 clip2 = mvp.transform_point(Vec4(v2.position, 1.0));

    if (clip0.w <= 0.0 || clip1.w <= 0.0 || clip2.w <= 0.0) {
        return;
    }

    auto to_screen = [&](const Vec4& clip) {
        Vec3 ndc = clip.perspective_divide();
        return Vec3{
            (ndc.x + 1.0) * 0.5 * width_,
            (1.0 - ndc.y) * 0.5 * height_,
            ndc.z,
        };
    };

    Vec3 p0 = to_screen(clip0);
    Vec3 p1 = to_screen(clip1);
    Vec3 p2 = to_screen(clip2);

    const int min_x = std::clamp(static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x}))), 0, width_ - 1);
    const int max_x = std::clamp(static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x}))), 0, width_ - 1);
    const int min_y = std::clamp(static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y}))), 0, height_ - 1);
    const int max_y = std::clamp(static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y}))), 0, height_ - 1);

    const double area = edge_function(p0, p1, p2);
    if (std::fabs(area) < 1e-8) {
        return;
    }

    const double inv_w0 = 1.0 / clip0.w;
    const double inv_w1 = 1.0 / clip1.w;
    const double inv_w2 = 1.0 / clip2.w;

    const double u0 = v0.uv.x * inv_w0;
    const double u1 = v1.uv.x * inv_w1;
    const double u2 = v2.uv.x * inv_w2;
    const double v0v = v0.uv.y * inv_w0;
    const double v1v = v1.uv.y * inv_w1;
    const double v2v = v2.uv.y * inv_w2;

    const double z0 = p0.z * inv_w0;
    const double z1 = p1.z * inv_w1;
    const double z2 = p2.z * inv_w2;

    const Vec3 n0 = v0.normal * inv_w0;
    const Vec3 n1 = v1.normal * inv_w1;
    const Vec3 n2 = v2.normal * inv_w2;

    const Vec3 light_dir = light.direction.normalized();

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            Vec3 p{static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, 0.0};
            const double w0 = edge_function(p1, p2, p) / area;
            const double w1 = edge_function(p2, p0, p) / area;
            const double w2 = edge_function(p0, p1, p) / area;

            if (w0 < 0.0 || w1 < 0.0 || w2 < 0.0) {
                continue;
            }

            const double inv_w = w0 * inv_w0 + w1 * inv_w1 + w2 * inv_w2;
            const double w = 1.0 / inv_w;

            const double depth = (w0 * z0 + w1 * z1 + w2 * z2) * w;
            const Vec2 uv{(w0 * u0 + w1 * u1 + w2 * u2) * w, (w0 * v0v + w1 * v1v + w2 * v2v) * w};
            Vec3 normal = (n0 * w0 + n1 * w1 + n2 * w2) * w;
            normal = normal.normalized();

            Color albedo = sample_texture(texture, uv);

            const double diff = std::max(0.0, normal.dot(-light_dir));
            Color shaded = albedo * (light.ambient + light.color * diff);
            set_pixel(x, y, depth, clamp_color(shaded));
        }
    }
}

void SoftwareRasterizer::draw_mesh(
    const Mesh& mesh,
    const Mat4& model,
    const Mat4& view,
    const Mat4& projection,
    const Light& light,
    const Texture* texture) {
    const Mat4 mvp = projection * view * model;
    for (const MeshTriangle& tri : mesh.triangles) {
        draw_triangle(mesh.vertices[tri.i0], mesh.vertices[tri.i1], mesh.vertices[tri.i2], mvp, light, texture);
    }
}

void SoftwareRasterizer::present(Framebuffer& framebuffer) const {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            framebuffer.set_pixel(x, y, color_buffer_[y * width_ + x]);
        }
    }
}

}  // namespace renderer
