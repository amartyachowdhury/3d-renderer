#pragma once

#include "renderer/raytracer/bvh.h"
#include "renderer/raytracer/hittable.h"
#include "renderer/raytracer/triangle.h"
#include "renderer/rasterizer/mesh.h"

#include <memory>
#include <string>
#include <vector>

namespace renderer {

std::unique_ptr<Hittable> build_bvh_from_mesh(const Mesh& mesh, Material* material);

}  // namespace renderer
