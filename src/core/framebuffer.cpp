#include "renderer/core/framebuffer.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace renderer {

Framebuffer::Framebuffer(int width, int height)
    : width_(width), height_(height), accumulation_(width * height), pixels_(width * height * 3, 0) {}

void Framebuffer::set_pixel(int x, int y, const Color& color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }

    const Color display = to_display_color(clamp_color(color));
    const int index = (y * width_ + x) * 3;
    pixels_[index + 0] = static_cast<uint8_t>(255.999 * display.x);
    pixels_[index + 1] = static_cast<uint8_t>(255.999 * display.y);
    pixels_[index + 2] = static_cast<uint8_t>(255.999 * display.z);
}

void Framebuffer::add_sample(int x, int y, const Color& color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }
    accumulation_[y * width_ + x] += color;
}

void Framebuffer::clear(const Color& color) {
    std::fill(accumulation_.begin(), accumulation_.end(), color);
    std::fill(pixels_.begin(), pixels_.end(), 0);
}

void Framebuffer::finalize_samples() {
    const double inv = 1.0 / static_cast<double>(sample_count_);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            set_pixel(x, y, accumulation_[y * width_ + x] * inv);
        }
    }
}

bool Framebuffer::write_ppm(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    out.write(reinterpret_cast<const char*>(pixels_.data()), static_cast<std::streamsize>(pixels_.size()));
    return static_cast<bool>(out);
}

}  // namespace renderer
