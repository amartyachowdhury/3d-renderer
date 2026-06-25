#include "renderer/platform/sdl_window.h"

#include <SDL.h>
#include <cstring>

namespace renderer {

SdlWindow::SdlWindow(const std::string& title, int width, int height)
    : width_(width), height_(height) {
    keys_down_.resize(SDL_NUM_SCANCODES, false);
    keys_just_pressed_.resize(SDL_NUM_SCANCODES, false);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        last_error_ = std::string("SDL_Init failed: ") + SDL_GetError();
        return;
    }

    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN);

    if (!window_) {
        last_error_ = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
        SDL_Quit();
        return;
    }

    renderer_ = SDL_CreateRenderer(static_cast<SDL_Window*>(window_), -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        last_error_ = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
        SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
        window_ = nullptr;
        SDL_Quit();
        return;
    }

    texture_ = SDL_CreateTexture(
        static_cast<SDL_Renderer*>(renderer_),
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);

    if (!texture_) {
        last_error_ = std::string("SDL_CreateTexture failed: ") + SDL_GetError();
        SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
        SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
        renderer_ = nullptr;
        window_ = nullptr;
        SDL_Quit();
        return;
    }

    open_ = true;
}

SdlWindow::~SdlWindow() {
    if (texture_) {
        SDL_DestroyTexture(static_cast<SDL_Texture*>(texture_));
    }
    if (renderer_) {
        SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
    }
    if (window_) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
    }
    SDL_Quit();
}

void SdlWindow::set_title(const std::string& title) {
    if (open_ && window_) {
        SDL_SetWindowTitle(static_cast<SDL_Window*>(window_), title.c_str());
    }
}

bool SdlWindow::resize(int width, int height) {
    if (!open_ || width <= 0 || height <= 0) {
        return false;
    }

    if (width == width_ && height == height_) {
        return true;
    }

    SDL_SetWindowSize(static_cast<SDL_Window*>(window_), width, height);
    SDL_DestroyTexture(static_cast<SDL_Texture*>(texture_));
    texture_ = SDL_CreateTexture(
        static_cast<SDL_Renderer*>(renderer_),
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);

    if (!texture_) {
        last_error_ = std::string("SDL_CreateTexture failed on resize: ") + SDL_GetError();
        return false;
    }

    width_ = width;
    height_ = height;
    return true;
}

void SdlWindow::blit(const Framebuffer& framebuffer) {
    if (!open_ || framebuffer.width() != width_ || framebuffer.height() != height_) {
        return;
    }

    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(
            static_cast<SDL_Texture*>(texture_),
            nullptr,
            &pixels,
            &pitch) != 0) {
        return;
    }

    const uint8_t* src = framebuffer.data();
    auto* dst = static_cast<uint8_t*>(pixels);
    for (int y = 0; y < height_; ++y) {
        std::memcpy(dst + y * pitch, src + y * width_ * 3, static_cast<size_t>(width_ * 3));
    }

    SDL_UnlockTexture(static_cast<SDL_Texture*>(texture_));
    SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(texture_), nullptr, nullptr);
    SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
}

void SdlWindow::poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            should_close_ = true;
        } else if (event.type == SDL_MOUSEMOTION) {
            mouse_delta_x_ += event.motion.xrel;
            mouse_delta_y_ += event.motion.yrel;
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            mouse_left_down_ = true;
        } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            mouse_left_down_ = false;
        } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
            const int scancode = event.key.keysym.scancode;
            if (scancode >= 0 && scancode < SDL_NUM_SCANCODES) {
                keys_down_[static_cast<size_t>(scancode)] = true;
                keys_just_pressed_[static_cast<size_t>(scancode)] = true;
            }
        } else if (event.type == SDL_KEYUP) {
            const int scancode = event.key.keysym.scancode;
            if (scancode >= 0 && scancode < SDL_NUM_SCANCODES) {
                keys_down_[static_cast<size_t>(scancode)] = false;
            }
        }
    }
}

bool SdlWindow::key_pressed(int scancode) const {
    if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) {
        return false;
    }
    return keys_down_[static_cast<size_t>(scancode)];
}

bool SdlWindow::key_just_pressed(int scancode) const {
    if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) {
        return false;
    }
    return keys_just_pressed_[static_cast<size_t>(scancode)];
}

void SdlWindow::clear_key_just_pressed() {
    std::fill(keys_just_pressed_.begin(), keys_just_pressed_.end(), false);
}

void SdlWindow::clear_mouse_delta() {
    mouse_delta_x_ = 0;
    mouse_delta_y_ = 0;
}

void SdlWindow::set_relative_mouse_mode(bool enabled) {
    relative_mouse_ = enabled;
    if (open_) {
        SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
    }
}

}  // namespace renderer
