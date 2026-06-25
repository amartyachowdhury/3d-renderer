#include "renderer/app/renderer_app.h"

#include "renderer/app/app_state.h"

#include <SDL.h>
#include <chrono>
#include <iostream>

namespace renderer {

RendererApp::RendererApp(AssetLocator assets) : assets_(std::move(assets)) {}

bool RendererApp::switch_mode(RendererMode mode, SdlWindow& window) {
    if (controller_) {
        if (mode == RendererMode::Raycaster) {
            window.set_relative_mouse_mode(false);
        }
        controller_->on_exit();
    }

    auto next = make_controller(mode);
    if (!next) {
        std::cerr << "Unknown renderer mode\n";
        return false;
    }

    std::string error;
    if (!next->init(assets_, error)) {
        std::cerr << "Failed to initialize " << renderer_mode_name(mode) << ": " << error << '\n';
        return false;
    }

    controller_ = std::move(next);
    current_mode_ = mode;
    controller_->on_enter();

    if (mode == RendererMode::Raycaster) {
        window.set_relative_mouse_mode(true);
    } else {
        window.set_relative_mouse_mode(false);
    }

    const int width = controller_->width();
    const int height = controller_->height();
    if (!window.resize(width, height)) {
        std::cerr << "Failed to resize window: " << window.last_error() << '\n';
        return false;
    }
    framebuffer_ = std::make_unique<Framebuffer>(width, height);
    return true;
}

void RendererApp::handle_global_input(SdlWindow& window, AppState& state) {
    if (window.key_just_pressed(SDL_SCANCODE_ESCAPE)) {
        window.poll_events();
    }

    if (window.key_just_pressed(SDL_SCANCODE_1)) {
        state.mode = RendererMode::Raytracer;
    } else if (window.key_just_pressed(SDL_SCANCODE_2)) {
        state.mode = RendererMode::Rasterizer;
    } else if (window.key_just_pressed(SDL_SCANCODE_3)) {
        state.mode = RendererMode::Raycaster;
    } else if (window.key_just_pressed(SDL_SCANCODE_R)) {
        if (controller_) {
            controller_->on_enter();
            state.status_message = "Reset " + std::string(renderer_mode_name(current_mode_));
        }
    } else if (window.key_just_pressed(SDL_SCANCODE_S)) {
        std::string error;
        if (framebuffer_ && save_current_frame(*framebuffer_, current_mode_, error)) {
            state.status_message = "Saved image";
        } else {
            state.status_message = error.empty() ? "Save failed" : error;
        }
    }
}

bool RendererApp::save_current_frame(const Framebuffer& framebuffer, RendererMode mode, std::string& error) {
    std::string filename;
    switch (mode) {
        case RendererMode::Raytracer:
            filename = "raytracer.png";
            break;
        case RendererMode::Rasterizer:
            filename = "rasterizer.png";
            break;
        case RendererMode::Raycaster:
            filename = "raycaster.png";
            break;
    }

    const std::string path = assets_.user_output_path(filename);
    if (!framebuffer.write_image(path)) {
        error = "Failed to write " + path;
        return false;
    }

    std::cout << "Saved " << path << '\n';
    return true;
}

void RendererApp::update_window_title(SdlWindow& window, const AppState& state) {
    std::string title = std::string("3D Renderer - ") + renderer_mode_name(current_mode_);
    if (!state.status_message.empty()) {
        title += " | " + state.status_message;
    }
    if (controller_) {
        title += " | " + controller_->help_text();
    }
    window.set_title(title);
}

int RendererApp::run() {
    AppState state;

    SdlWindow window("3D Renderer", 960, 540);
    if (!window.is_open()) {
        std::cerr << "Failed to open SDL window";
        if (!window.last_error().empty()) {
            std::cerr << ": " << window.last_error();
        }
        std::cerr << '\n';
        return 1;
    }

    if (!switch_mode(state.mode, window)) {
        return 1;
    }

    state.status_message = "Press 1/2/3 to switch modes, S to save, R to reset, Esc to quit";

    auto last_time = std::chrono::steady_clock::now();
    RendererMode pending_mode = state.mode;

    while (window.is_open() && !window.should_close()) {
        window.poll_events();

        if (window.key_pressed(SDL_SCANCODE_ESCAPE)) {
            break;
        }

        handle_global_input(window, state);

        if (state.mode != current_mode_) {
            pending_mode = state.mode;
        }

        if (pending_mode != current_mode_) {
            if (switch_mode(pending_mode, window)) {
                state.status_message = "Switched to " + std::string(renderer_mode_name(current_mode_));
            } else {
                state.mode = current_mode_;
                pending_mode = current_mode_;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const double dt = std::chrono::duration<double>(now - last_time).count();
        last_time = now;

        if (controller_ && framebuffer_) {
            controller_->update(window, dt);
            controller_->render(*framebuffer_);
            window.blit(*framebuffer_);
            update_window_title(window, state);
        }

        window.clear_key_just_pressed();
    }

    if (controller_) {
        controller_->on_exit();
        window.set_relative_mouse_mode(false);
    }

    return 0;
}

}  // namespace renderer
