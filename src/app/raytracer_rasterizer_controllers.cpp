#include "renderer/app/renderer_controller.h"

#include "renderer/core/framebuffer.h"
#include "renderer/core/texture.h"
#include "renderer/math/constants.h"
#include "renderer/math/mat4.h"
#include "renderer/platform/sdl_window.h"
#include "renderer/rasterizer/mesh.h"
#include "renderer/rasterizer/rasterizer.h"
#include "renderer/raytracer/material.h"
#include "renderer/raytracer/mesh_bvh.h"
#include "renderer/raytracer/scene_loader.h"
#include "renderer/raycaster/map.h"
#include "renderer/raycaster/raycaster_engine.h"

#include <SDL.h>
#include <memory>
#include <string>
#include <vector>

namespace renderer {

namespace {

class RaytracerController final : public RendererController {
public:
    bool init(const AssetLocator& assets, std::string& error) override {
        assets_ = &assets;
        return load_scene_data(error);
    }

    void on_enter() override {
        accumulated_samples_ = 0;
        target_samples_ = 64;
        if (framebuffer_) {
            framebuffer_->clear({0.0, 0.0, 0.0});
        }
    }

    void update(SdlWindow& window, double /*dt*/) override {
        if (window.mouse_left_down()) {
            camera_yaw_ += window.mouse_delta_x() * 0.008;
            window.clear_mouse_delta();
        } else {
            window.clear_mouse_delta();
        }

        if (accumulated_samples_ < target_samples_) {
            render_samples(1);
        }
    }

    void render(Framebuffer& framebuffer) override {
        framebuffer_ = &framebuffer;
        if (accumulated_samples_ == 0) {
            framebuffer.clear({0.0, 0.0, 0.0});
            render_samples(1);
        }
        framebuffer.finalize_samples();
    }

    int width() const override { return scene_desc_.width; }
    int height() const override { return scene_desc_.height; }

    std::string mode_label() const override { return "Ray Tracer"; }

    std::string help_text() const override {
        return "Drag: orbit camera | R: reset samples | Samples: " + std::to_string(accumulated_samples_) + "/" +
               std::to_string(target_samples_);
    }

private:
    bool load_scene_data(std::string& error) {
        const std::string scene_path = assets_->resolve("scenes/cornell.scene");
        if (!load_scene(scene_path, scene_desc_, error)) {
            return false;
        }

        scene_desc_.width = 640;
        scene_desc_.height = 360;
        scene_desc_.samples = 1;
        scene_desc_.max_depth = 8;

        materials_.clear();
        scene_textures_.clear();
        HittableList list;
        build_scene(scene_desc_, list, materials_, scene_textures_);
        world_ = build_bvh_world(std::move(list));
        return true;
    }

    Camera make_camera() const {
        const double aspect = static_cast<double>(scene_desc_.width) / static_cast<double>(scene_desc_.height);
        const double yaw = camera_yaw_ * kPi / 180.0;
        const double radius = (scene_desc_.look_from - scene_desc_.look_at).length();
        const Point3 look_from{
            scene_desc_.look_at.x + radius * std::sin(yaw),
            scene_desc_.look_from.y,
            scene_desc_.look_at.z + radius * std::cos(yaw),
        };
        return Camera(
            look_from,
            scene_desc_.look_at,
            scene_desc_.vup,
            scene_desc_.vfov,
            aspect,
            scene_desc_.aperture,
            scene_desc_.focus_dist);
    }

    void render_samples(int count) {
        if (!framebuffer_ || !world_) {
            return;
        }

        const Camera camera = make_camera();
        for (int sample = 0; sample < count; ++sample) {
            for (int y = 0; y < scene_desc_.height; ++y) {
                for (int x = 0; x < scene_desc_.width; ++x) {
                    const double jitter_x = random_double();
                    const double jitter_y = random_double();
                    const double u = (x + jitter_x) / (scene_desc_.width - 1);
                    const double v = (y + jitter_y) / (scene_desc_.height - 1);
                    const Ray ray = camera.get_ray(u, v);
                    const Color color = ray_color(ray, *world_, scene_desc_.max_depth);
                    framebuffer_->add_sample(x, scene_desc_.height - 1 - y, color);
                }
            }
            framebuffer_->end_sample_pass();
            ++accumulated_samples_;
        }
    }

    const AssetLocator* assets_ = nullptr;
    SceneDescription scene_desc_;
    std::vector<std::unique_ptr<Material>> materials_;
    std::vector<Texture> scene_textures_;
    std::unique_ptr<Hittable> world_;
    Framebuffer* framebuffer_ = nullptr;
    double camera_yaw_ = 0.0;
    int accumulated_samples_ = 0;
    int target_samples_ = 64;
};

class RasterizerController final : public RendererController {
public:
    bool init(const AssetLocator& assets, std::string& error) override {
        assets_ = &assets;
        const std::string obj_path = assets.resolve("models/cube.obj");
        if (!load_obj(obj_path, mesh_, error)) {
            mesh_ = make_cube();
        }

        const std::string texture_path = assets.resolve("textures/checker.png");
        if (load_texture(texture_path, texture_, error)) {
            texture_ptr_ = &texture_;
        } else {
            texture_ptr_ = nullptr;
            error.clear();
        }

        rasterizer_ = std::make_unique<SoftwareRasterizer>(width_, height_);
        return true;
    }

    void on_enter() override {
        orbit_yaw_ = 0.8;
        orbit_pitch_ = 0.35;
        auto_spin_ = 0.0;
        wireframe_ = false;
        debug_mode_index_ = 0;
        apply_debug_mode();
    }

    void update(SdlWindow& window, double /*dt*/) override {
        if (window.key_just_pressed(SDL_SCANCODE_F)) {
            wireframe_ = !wireframe_;
        }
        if (window.key_just_pressed(SDL_SCANCODE_D)) {
            debug_mode_index_ = (debug_mode_index_ + 1) % 4;
            apply_debug_mode();
        }

        if (window.mouse_left_down()) {
            orbit_yaw_ += window.mouse_delta_x() * 0.008;
            orbit_pitch_ += window.mouse_delta_y() * 0.008;
            orbit_pitch_ = std::clamp(orbit_pitch_, -1.4, 1.4);
            auto_spin_ = 0.0;
        } else {
            auto_spin_ += 0.015;
        }
        window.clear_mouse_delta();
    }

    void render(Framebuffer& framebuffer) override {
        const double yaw = orbit_yaw_ + auto_spin_;
        const Point3 eye{
            target_.x + orbit_distance_ * std::cos(orbit_pitch_) * std::sin(yaw),
            target_.y + orbit_distance_ * std::sin(orbit_pitch_),
            target_.z + orbit_distance_ * std::cos(orbit_pitch_) * std::cos(yaw),
        };

        const Mat4 view = Mat4::look_at(eye, target_, up_);
        const Mat4 projection = Mat4::perspective(60.0 * kPi / 180.0, static_cast<double>(width_) / height_, 0.1, 100.0);
        const Mat4 model = Mat4::rotation_y(yaw * 0.5);

        rasterizer_->clear({0.05, 0.07, 0.12});
        rasterizer_->draw_mesh(mesh_, model, view, projection, Light{}, texture_ptr_, eye);
        if (wireframe_) {
            rasterizer_->draw_wireframe_mesh(mesh_, model, view, projection);
        }
        rasterizer_->present(framebuffer);
    }

    int width() const override { return width_; }
    int height() const override { return height_; }

    std::string mode_label() const override { return "Rasterizer"; }

    std::string help_text() const override {
        return "Drag: orbit | F: wireframe | D: debug view";
    }

private:
    void apply_debug_mode() {
        switch (debug_mode_index_) {
            case 1:
                rasterizer_->set_debug_mode(DebugMode::Depth);
                break;
            case 2:
                rasterizer_->set_debug_mode(DebugMode::Normal);
                break;
            case 3:
                rasterizer_->set_debug_mode(DebugMode::Uv);
                break;
            default:
                rasterizer_->set_debug_mode(DebugMode::Shaded);
                break;
        }
    }

    const AssetLocator* assets_ = nullptr;
    Mesh mesh_;
    Texture texture_;
    const Texture* texture_ptr_ = nullptr;
    std::unique_ptr<SoftwareRasterizer> rasterizer_;
    Point3 target_{0.0, 0.0, 0.0};
    Vec3 up_{0.0, 1.0, 0.0};
    double orbit_yaw_ = 0.8;
    double orbit_pitch_ = 0.35;
    double orbit_distance_ = 4.0;
    double auto_spin_ = 0.0;
    bool wireframe_ = false;
    int debug_mode_index_ = 0;
    int width_ = 960;
    int height_ = 540;
};

bool can_move(const Map& map, double x, double y) {
    return map.tile(static_cast<int>(x), static_cast<int>(y)) == 0;
}

std::vector<std::string> default_map_rows() {
    return {
        "111111111111",
        "100000000001",
        "101111101001",
        "101000101001",
        "101011101001",
        "100010000001",
        "111111111111",
    };
}

std::vector<Texture> load_wall_textures(const AssetLocator& assets, std::string& error) {
    std::vector<Texture> textures;
    const char* names[] = {"wall1.png", "wall2.png", "wall3.png", "sprite.png"};
    for (const char* name : names) {
        Texture texture;
        const std::string path = assets.resolve(std::string("textures/") + name);
        if (!load_texture(path, texture, error)) {
            return {};
        }
        textures.push_back(std::move(texture));
    }
    return textures;
}

class RaycasterController final : public RendererController {
public:
    bool init(const AssetLocator& assets, std::string& error) override {
        assets_ = &assets;

        std::vector<std::string> rows;
        const std::string map_path = assets.resolve("maps/level1.txt");
        if (!load_map(map_path, rows, error)) {
            rows = default_map_rows();
            error.clear();
        }

        map_ = std::make_unique<Map>(rows);
        settings_.width = 960;
        settings_.height = 540;
        engine_ = std::make_unique<RaycasterEngine>(*map_, settings_);

        auto wall_textures = load_wall_textures(assets, error);
        if (wall_textures.empty()) {
            wall_textures = {
                make_checker_texture(64, 8, {0.8, 0.25, 0.25}, {0.45, 0.12, 0.12}),
                make_checker_texture(64, 8, {0.25, 0.75, 0.35}, {0.12, 0.45, 0.18}),
                make_checker_texture(64, 8, {0.25, 0.45, 0.85}, {0.12, 0.25, 0.55}),
                make_checker_texture(64, 4, {0.9, 0.25, 0.25}, {0.15, 0.15, 0.15}),
            };
            error.clear();
        }
        engine_->set_wall_textures(std::move(wall_textures));

        SpawnData spawn;
        const std::string spawn_path = assets.resolve("maps/level1.spawn");
        if (!load_spawn(spawn_path, spawn, error)) {
            spawn.sprites = {
                {4.5, 3.5, 3},
                {8.5, 8.5, 3},
                {12.5, 5.5, 3},
                {6.5, 11.5, 3},
            };
            error.clear();
        }
        engine_->set_sprites(std::move(spawn.sprites));
        player_ = spawn.player;
        return true;
    }

    void on_enter() override {
        capture_mouse_ = true;
    }

    void on_exit() override {
        capture_mouse_ = false;
    }

    void update(SdlWindow& window, double /*dt*/) override {
        if (capture_mouse_ && !window.relative_mouse_mode()) {
            window.set_relative_mouse_mode(true);
        }

        if (window.key_just_pressed(SDL_SCANCODE_M)) {
            capture_mouse_ = !capture_mouse_;
            window.set_relative_mouse_mode(capture_mouse_);
        }

        if (window.mouse_delta_x() != 0 || window.mouse_delta_y() != 0) {
            player_.angle += window.mouse_delta_x() * mouse_sensitivity_;
            window.clear_mouse_delta();
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        const double forward_x = std::cos(player_.angle);
        const double forward_y = std::sin(player_.angle);
        const double strafe_x = std::cos(player_.angle + kPi / 2.0);
        const double strafe_y = std::sin(player_.angle + kPi / 2.0);

        if (keys[SDL_SCANCODE_W]) {
            double nx = player_.x + forward_x * move_speed_;
            double ny = player_.y + forward_y * move_speed_;
            if (can_move(*map_, nx, player_.y)) player_.x = nx;
            if (can_move(*map_, player_.x, ny)) player_.y = ny;
        }
        if (keys[SDL_SCANCODE_S]) {
            double nx = player_.x - forward_x * move_speed_;
            double ny = player_.y - forward_y * move_speed_;
            if (can_move(*map_, nx, player_.y)) player_.x = nx;
            if (can_move(*map_, player_.x, ny)) player_.y = ny;
        }
        if (keys[SDL_SCANCODE_A]) {
            double nx = player_.x - strafe_x * move_speed_;
            double ny = player_.y - strafe_y * move_speed_;
            if (can_move(*map_, nx, player_.y)) player_.x = nx;
            if (can_move(*map_, player_.x, ny)) player_.y = ny;
        }
        if (keys[SDL_SCANCODE_D]) {
            double nx = player_.x + strafe_x * move_speed_;
            double ny = player_.y + strafe_y * move_speed_;
            if (can_move(*map_, nx, player_.y)) player_.x = nx;
            if (can_move(*map_, player_.x, ny)) player_.y = ny;
        }
        if (keys[SDL_SCANCODE_LEFT]) {
            player_.angle -= rot_speed_;
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            player_.angle += rot_speed_;
        }
    }

    void render(Framebuffer& framebuffer) override {
        engine_->render(player_, framebuffer);
        engine_->render_minimap(player_, framebuffer);
    }

    int width() const override { return settings_.width; }
    int height() const override { return settings_.height; }

    std::string mode_label() const override { return "Raycaster"; }

    std::string help_text() const override {
        return "WASD: move | Arrows/mouse: turn | M: toggle mouse capture";
    }

private:
    const AssetLocator* assets_ = nullptr;
    std::unique_ptr<Map> map_;
    RaycasterSettings settings_;
    std::unique_ptr<RaycasterEngine> engine_;
    Player player_;
    bool capture_mouse_ = false;
    double move_speed_ = 0.06;
    double rot_speed_ = 0.04;
    double mouse_sensitivity_ = 0.003;
};

}  // namespace

std::unique_ptr<RendererController> make_controller(RendererMode mode) {
    switch (mode) {
        case RendererMode::Raytracer:
            return std::make_unique<RaytracerController>();
        case RendererMode::Rasterizer:
            return std::make_unique<RasterizerController>();
        case RendererMode::Raycaster:
            return std::make_unique<RaycasterController>();
    }
    return nullptr;
}

}  // namespace renderer
