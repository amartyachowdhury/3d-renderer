#pragma once

#include <string>
#include <vector>

namespace renderer {

class Map {
public:
    explicit Map(const std::vector<std::string>& rows);

    int width() const { return width_; }
    int height() const { return height_; }
    int tile(int x, int y) const;

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<int> tiles_;
};

}  // namespace renderer
