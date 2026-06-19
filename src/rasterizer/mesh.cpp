#include "renderer/rasterizer/mesh.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace renderer {

Mesh make_cube() {
    Mesh mesh;
    mesh.vertices = {
        {{-1, -1, -1}, {0, 0, -1}, {0, 0}},
        {{1, -1, -1}, {0, 0, -1}, {1, 0}},
        {{1, 1, -1}, {0, 0, -1}, {1, 1}},
        {{-1, 1, -1}, {0, 0, -1}, {0, 1}},
        {{-1, -1, 1}, {0, 0, 1}, {0, 0}},
        {{1, -1, 1}, {0, 0, 1}, {1, 0}},
        {{1, 1, 1}, {0, 0, 1}, {1, 1}},
        {{-1, 1, 1}, {0, 0, 1}, {0, 1}},
    };

    mesh.triangles = {
        {0, 1, 2}, {2, 3, 0},
        {4, 6, 5}, {6, 4, 7},
        {0, 4, 5}, {5, 1, 0},
        {2, 6, 7}, {7, 3, 2},
        {0, 3, 7}, {7, 4, 0},
        {1, 5, 6}, {6, 2, 1},
    };

    return mesh;
}

bool load_obj(const std::string& path, Mesh& mesh, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "Unable to open OBJ: " + path;
        return false;
    }

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    struct FaceVertex { int p = -1; int t = -1; int n = -1; };
    std::vector<std::vector<FaceVertex>> faces;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream stream(line);
        std::string prefix;
        stream >> prefix;

        if (prefix == "v") {
            Vec3 v;
            stream >> v.x >> v.y >> v.z;
            positions.push_back(v);
        } else if (prefix == "vn") {
            Vec3 n;
            stream >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (prefix == "vt") {
            Vec2 t;
            stream >> t.x >> t.y;
            uvs.push_back(t);
        } else if (prefix == "f") {
            std::vector<FaceVertex> face;
            std::string token;
            while (stream >> token) {
                FaceVertex fv;
                std::replace(token.begin(), token.end(), '/', ' ');
                std::istringstream face_stream(token);
                face_stream >> fv.p;
                if (!(face_stream >> fv.t)) {
                    fv.t = -1;
                }
                if (!(face_stream >> fv.n)) {
                    fv.n = -1;
                }
                if (fv.p > 0) {
                    --fv.p;
                }
                if (fv.t > 0) {
                    --fv.t;
                }
                if (fv.n > 0) {
                    --fv.n;
                }
                face.push_back(fv);
            }
            if (face.size() >= 3) {
                faces.push_back(face);
            }
        }
    }

    mesh.vertices.clear();
    mesh.triangles.clear();
    std::unordered_map<std::string, int> dedupe;

    auto index_for = [&](const FaceVertex& fv) {
        const std::string key = std::to_string(fv.p) + "/" + std::to_string(fv.t) + "/" + std::to_string(fv.n);
        if (dedupe.count(key)) {
            return dedupe[key];
        }

        Vertex vertex;
        vertex.position = positions.at(fv.p);
        vertex.normal = fv.n >= 0 ? normals.at(fv.n) : Vec3{0, 1, 0};
        vertex.uv = fv.t >= 0 ? uvs.at(fv.t) : Vec2{0, 0};
        const int index = static_cast<int>(mesh.vertices.size());
        mesh.vertices.push_back(vertex);
        dedupe[key] = index;
        return index;
    };

    for (const auto& face : faces) {
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            mesh.triangles.push_back({
                index_for(face[0]),
                index_for(face[i]),
                index_for(face[i + 1]),
            });
        }
    }

    return !mesh.vertices.empty();
}

}  // namespace renderer
