#include "renderer/raycaster/map.h"

#include <algorithm>
#include <fstream>

namespace renderer {

Map::Map(const std::vector<std::string>& rows) : height_(static_cast<int>(rows.size())) {
    width_ = 0;
    for (const auto& row : rows) {
        width_ = std::max(width_, static_cast<int>(row.size()));
    }

    tiles_.assign(width_ * height_, 0);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            char c = x < static_cast<int>(rows[y].size()) ? rows[y][x] : ' ';
            tiles_[y * width_ + x] = c == ' ' ? 0 : c - '0';
        }
    }
}

int Map::tile(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return 1;
    }
    return tiles_[y * width_ + x];
}

bool load_map(const std::string& path, std::vector<std::string>& rows, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "Unable to open map: " + path;
        return false;
    }

    rows.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            rows.push_back(line);
        }
    }

    if (rows.empty()) {
        error = "Map file is empty: " + path;
        return false;
    }

    return true;
}

}  // namespace renderer
