#pragma once

#include "renderer/core/framebuffer.h"

#include <string>
#include <vector>

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
    void set_title(const std::string& title);
    bool resize(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    int mouse_delta_x() const { return mouse_delta_x_; }
    int mouse_delta_y() const { return mouse_delta_y_; }
    bool mouse_left_down() const { return mouse_left_down_; }
    void clear_mouse_delta();
    void set_relative_mouse_mode(bool enabled);
    bool relative_mouse_mode() const { return relative_mouse_; }

    bool key_pressed(int scancode) const;
    bool key_just_pressed(int scancode) const;
    void clear_key_just_pressed();

    const std::string& last_error() const { return last_error_; }

private:
    bool open_ = false;
    bool should_close_ = false;
    bool mouse_left_down_ = false;
    bool relative_mouse_ = false;
    int mouse_delta_x_ = 0;
    int mouse_delta_y_ = 0;
    std::string last_error_;
    std::vector<bool> keys_down_;
    std::vector<bool> keys_just_pressed_;
    void* window_ = nullptr;
    void* renderer_ = nullptr;
    void* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace renderer
