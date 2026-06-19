#pragma once

#include "renderer/core/color.h"
#include "renderer/math/vec2.h"

#include <cstdint>
#include <string>
#include <vector>

namespace renderer {

struct Texture {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgba;
};

bool load_texture(const std::string& path, Texture& texture, std::string& error);
bool load_ppm_texture(const std::string& path, Texture& texture, std::string& error);
Color sample_texture(const Texture* texture, const Vec2& uv);
Texture make_checker_texture(int size, int cells, Color a, Color b);

}  // namespace renderer
