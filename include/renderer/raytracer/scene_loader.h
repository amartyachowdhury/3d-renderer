#pragma once

#include "renderer/raytracer/hittable.h"

#include <memory>
#include <string>
#include <vector>

namespace renderer {

struct SceneSphere {
    Point3 center;
    double radius;
    std::string material_type;
    Color albedo;
    double fuzz = 0.0;
    double ior = 1.5;
};

struct SceneDescription {
    int width = 800;
    int height = 450;
    int samples = 10;
    int max_depth = 50;
    Point3 look_from{13.0, 2.0, 3.0};
    Point3 look_at{0.0, 0.0, 0.0};
    Vec3 vup{0.0, 1.0, 0.0};
    double vfov = 20.0;
    double aperture = 0.0;
    double focus_dist = 10.0;
    std::vector<SceneSphere> spheres;
};

bool load_scene(const std::string& path, SceneDescription& scene, std::string& error);
void build_scene(const SceneDescription& description, HittableList& world, std::vector<std::unique_ptr<Material>>& materials);

HittableList build_default_scene(std::vector<std::unique_ptr<Material>>& materials);

}  // namespace renderer
