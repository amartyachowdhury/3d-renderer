#include "renderer/core/texture.h"
#include "renderer/core/path.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <fstream>

namespace renderer {

bool load_ppm_texture(const std::string& path, Texture& texture, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "Unable to open texture: " + path;
        return false;
    }

    std::string magic;
    in >> magic;
    if (magic != "P6" && magic != "P3") {
        error = "Only PPM textures (P3/P6) are supported";
        return false;
    }

    std::string line;
    while (in.peek() == '#') {
        std::getline(in, line);
    }

    in >> texture.width >> texture.height;
    int max_value = 0;
    in >> max_value;
    in.get();

    texture.rgba.resize(static_cast<size_t>(texture.width * texture.height * 4));
    for (int i = 0; i < texture.width * texture.height; ++i) {
        uint8_t rgb[3];
        if (magic == "P6") {
            in.read(reinterpret_cast<char*>(rgb), 3);
        } else {
            int r = 0;
            int g = 0;
            int b = 0;
            in >> r >> g >> b;
            rgb[0] = static_cast<uint8_t>(r);
            rgb[1] = static_cast<uint8_t>(g);
            rgb[2] = static_cast<uint8_t>(b);
        }
        texture.rgba[i * 4 + 0] = rgb[0];
        texture.rgba[i * 4 + 1] = rgb[1];
        texture.rgba[i * 4 + 2] = rgb[2];
        texture.rgba[i * 4 + 3] = 255;
    }

    return true;
}

bool load_texture(const std::string& path, Texture& texture, std::string& error) {
    const std::string ext = extension_lower(path);
    if (ext == "ppm" || ext == "pgm" || ext == "pbm") {
        return load_ppm_texture(path, texture, error);
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        error = std::string("Unable to load texture: ") + path + " (" + stbi_failure_reason() + ")";
        return false;
    }

    texture.width = width;
    texture.height = height;
    texture.rgba.assign(data, data + static_cast<size_t>(width * height * 4));
    stbi_image_free(data);
    return true;
}

Color sample_texture(const Texture* texture, const Vec2& uv) {
    if (!texture || texture->rgba.empty()) {
        return {1.0, 1.0, 1.0};
    }

    const double u = uv.x - std::floor(uv.x);
    const double v = uv.y - std::floor(uv.y);
    const int x = std::clamp(static_cast<int>(u * texture->width), 0, texture->width - 1);
    const int y = std::clamp(static_cast<int>((1.0 - v) * texture->height), 0, texture->height - 1);
    const int index = (y * texture->width + x) * 4;
    return {
        texture->rgba[index + 0] / 255.0,
        texture->rgba[index + 1] / 255.0,
        texture->rgba[index + 2] / 255.0,
    };
}

Texture make_checker_texture(int size, int cells, Color a, Color b) {
    Texture texture;
    texture.width = size;
    texture.height = size;
    texture.rgba.resize(static_cast<size_t>(size * size * 4));

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const bool checker = ((x / (size / cells)) + (y / (size / cells))) % 2 == 0;
            const Color c = checker ? a : b;
            const int index = (y * size + x) * 4;
            texture.rgba[index + 0] = static_cast<uint8_t>(255.999 * c.x);
            texture.rgba[index + 1] = static_cast<uint8_t>(255.999 * c.y);
            texture.rgba[index + 2] = static_cast<uint8_t>(255.999 * c.z);
            texture.rgba[index + 3] = 255;
        }
    }

    return texture;
}

}  // namespace renderer
