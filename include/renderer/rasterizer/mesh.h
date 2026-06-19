#pragma once

#include "renderer/math/vec2.h"
#include "renderer/math/vec3.h"

#include <string>
#include <vector>

namespace renderer {

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

struct MeshTriangle {
    int i0 = 0;
    int i1 = 0;
    int i2 = 0;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<MeshTriangle> triangles;
};

bool load_obj(const std::string& path, Mesh& mesh, std::string& error);
Mesh make_cube();

}  // namespace renderer
