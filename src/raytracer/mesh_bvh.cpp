#include "renderer/raytracer/mesh_bvh.h"

namespace renderer {

std::unique_ptr<Hittable> build_bvh_from_mesh(const Mesh& mesh, Material* material) {
    std::vector<std::unique_ptr<Hittable>> triangles;
    triangles.reserve(mesh.triangles.size());

    for (const auto& tri : mesh.triangles) {
        const Vertex& v0 = mesh.vertices[tri.i0];
        const Vertex& v1 = mesh.vertices[tri.i1];
        const Vertex& v2 = mesh.vertices[tri.i2];
        triangles.push_back(std::make_unique<Triangle>(v0.position, v1.position, v2.position, material));
    }

    return build_bvh(std::move(triangles));
}

}  // namespace renderer
