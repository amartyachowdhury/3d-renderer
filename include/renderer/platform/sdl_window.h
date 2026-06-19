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

private:
    bool open_ = false;
    bool should_close_ = false;
    void* window_ = nullptr;
    void* renderer_ = nullptr;
    void* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace renderer
