#pragma once

#include "renderer/core/framebuffer.h"

#include <string>

namespace renderer {

class SdlWindow {
public:
    SdlWindow(const std::string& title, int width, int height);
    ~SdlWindow();

    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;

    bool is_open() const { return open_; }
    void blit(const Framebuffer& framebuffer);
    void poll_events();
    bool should_close() const { return should_close_; }

    int mouse_delta_x() const { return mouse_delta_x_; }
    int mouse_delta_y() const { return mouse_delta_y_; }
    bool mouse_left_down() const { return mouse_left_down_; }
    void clear_mouse_delta();

private:
    bool open_ = false;
    bool should_close_ = false;
    bool mouse_left_down_ = false;
    int mouse_delta_x_ = 0;
    int mouse_delta_y_ = 0;
    void* window_ = nullptr;
    void* renderer_ = nullptr;
    void* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace renderer
