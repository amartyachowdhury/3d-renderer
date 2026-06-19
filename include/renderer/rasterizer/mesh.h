#pragma once

#include "renderer/math/vec2.h"
#include "renderer/math/vec3.h"

#include <cstdint>
#include <string>
#include <vector>

namespace renderer {

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

struct Triangle {
    int i0 = 0;
    int i1 = 0;
    int i2 = 0;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
};

struct Texture {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgba;
};

bool load_obj(const std::string& path, Mesh& mesh, std::string& error);
bool load_ppm_texture(const std::string& path, Texture& texture, std::string& error);
Mesh make_cube();

}  // namespace renderer
