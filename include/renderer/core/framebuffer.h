#pragma once

#include "renderer/core/color.h"

#include <cstdint>
#include <string>
#include <vector>

namespace renderer {

class Framebuffer {
public:
    Framebuffer(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    void set_pixel(int x, int y, const Color& color);
    void add_sample(int x, int y, const Color& color);
    void end_sample_pass();
    void clear(const Color& color = {0.0, 0.0, 0.0});
    void finalize_samples();

    const std::vector<uint8_t>& pixels() const { return pixels_; }
    const uint8_t* data() const { return pixels_.data(); }

    bool write_ppm(const std::string& path) const;
    bool write_png(const std::string& path) const;
    bool write_image(const std::string& path) const;

private:
    int width_;
    int height_;
    std::vector<Color> accumulation_;
    std::vector<uint8_t> pixels_;
    int sample_count_ = 1;
};

}  // namespace renderer
