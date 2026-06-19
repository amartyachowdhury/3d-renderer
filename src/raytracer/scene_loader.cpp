#include "renderer/raytracer/scene_loader.h"

#include "renderer/raytracer/material.h"

#include <fstream>
#include <sstream>

namespace renderer {

namespace {

std::string trim(const std::string& value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

}  // namespace

bool load_scene(const std::string& path, SceneDescription& scene, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "Unable to open scene file: " + path;
        return false;
    }

    scene = SceneDescription{};
    scene.spheres.clear();

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream stream(line);
        std::string key;
        stream >> key;

        if (key == "resolution") {
            stream >> scene.width >> scene.height;
        } else if (key == "samples") {
            stream >> scene.samples;
        } else if (key == "max_depth") {
            stream >> scene.max_depth;
        } else if (key == "camera") {
            stream >> scene.look_from.x >> scene.look_from.y >> scene.look_from.z;
            stream >> scene.look_at.x >> scene.look_at.y >> scene.look_at.z;
            stream >> scene.vup.x >> scene.vup.y >> scene.vup.z;
            stream >> scene.vfov >> scene.aperture >> scene.focus_dist;
        } else if (key == "sphere") {
            SceneSphere sphere;
            stream >> sphere.center.x >> sphere.center.y >> sphere.center.z >> sphere.radius >> sphere.material_type;

            if (sphere.material_type == "lambertian" || sphere.material_type == "metal") {
                stream >> sphere.albedo.x >> sphere.albedo.y >> sphere.albedo.z;
            }
            if (sphere.material_type == "metal") {
                stream >> sphere.fuzz;
            }
            if (sphere.material_type == "dielectric") {
                stream >> sphere.ior;
            }

            scene.spheres.push_back(sphere);
        } else {
            error = "Unknown scene directive: " + key;
            return false;
        }
    }

    return true;
}

void build_scene(const SceneDescription& description, HittableList& world, std::vector<std::unique_ptr<Material>>& materials) {
    for (const auto& sphere : description.spheres) {
        Material* material = nullptr;
        if (sphere.material_type == "lambertian") {
            materials.push_back(std::make_unique<Lambertian>(sphere.albedo));
            material = materials.back().get();
        } else if (sphere.material_type == "metal") {
            materials.push_back(std::make_unique<Metal>(sphere.albedo, sphere.fuzz));
            material = materials.back().get();
        } else if (sphere.material_type == "dielectric") {
            materials.push_back(std::make_unique<Dielectric>(sphere.ior));
            material = materials.back().get();
        }

        if (material) {
            world.add(std::make_unique<Sphere>(sphere.center, sphere.radius, material));
        }
    }
}

HittableList build_default_scene(std::vector<std::unique_ptr<Material>>& materials) {
    HittableList world;

    auto ground_material = std::make_unique<Lambertian>(Color{0.5, 0.5, 0.5});
    Material* ground = ground_material.get();
    materials.push_back(std::move(ground_material));
    world.add(std::make_unique<Sphere>(Point3{0.0, -1000.0, 0.0}, 1000.0, ground));

    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            const double choose_mat = random_double();
            Point3 center{a + 0.9 * random_double(), 0.2, b + 0.9 * random_double()};

            if ((center - Point3{4.0, 0.2, 0.0}).length() > 0.9) {
                Material* sphere_material = nullptr;
                if (choose_mat < 0.8) {
                    Color albedo{random_double() * random_double(), random_double() * random_double(), random_double() * random_double()};
                    materials.push_back(std::make_unique<Lambertian>(albedo));
                    sphere_material = materials.back().get();
                } else if (choose_mat < 0.95) {
                    Color albedo{0.5 * (1.0 + random_double()), 0.5 * (1.0 + random_double()), 0.5 * (1.0 + random_double())};
                    double fuzz = 0.5 * random_double();
                    materials.push_back(std::make_unique<Metal>(albedo, fuzz));
                    sphere_material = materials.back().get();
                } else {
                    materials.push_back(std::make_unique<Dielectric>(1.5));
                    sphere_material = materials.back().get();
                }
                world.add(std::make_unique<Sphere>(center, 0.2, sphere_material));
            }
        }
    }

    materials.push_back(std::make_unique<Dielectric>(1.5));
    world.add(std::make_unique<Sphere>(Point3{0.0, 1.0, 0.0}, 1.0, materials.back().get()));

    materials.push_back(std::make_unique<Lambertian>(Color{0.4, 0.2, 0.1}));
    world.add(std::make_unique<Sphere>(Point3{-4.0, 1.0, 0.0}, 1.0, materials.back().get()));

    materials.push_back(std::make_unique<Metal>(Color{0.7, 0.6, 0.5}, 0.0));
    world.add(std::make_unique<Sphere>(Point3{4.0, 1.0, 0.0}, 1.0, materials.back().get()));

    return world;
}

}  // namespace renderer
