#pragma once

#include <string>
#include <vector>

namespace renderer {

struct Player {
    double x = 2.5;
    double y = 2.5;
    double angle = 0.0;
};

struct Sprite {
    double x = 0.0;
    double y = 0.0;
    int texture_id = 0;
};

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

struct SpawnData {
    Player player{2.5, 2.5, 0.0};
    std::vector<Sprite> sprites;
};

bool load_map(const std::string& path, std::vector<std::string>& rows, std::string& error);
bool load_spawn(const std::string& path, SpawnData& spawn, std::string& error);

}  // namespace renderer
