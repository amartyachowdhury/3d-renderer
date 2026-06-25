#pragma once

#include <string>
#include <vector>

namespace renderer {

class AssetLocator {
public:
    AssetLocator();

    void add_search_root(const std::string& path);
    std::string resolve(const std::string& relative_path) const;
    bool exists(const std::string& relative_path) const;
    std::string user_output_path(const std::string& filename) const;

    const std::vector<std::string>& search_roots() const { return search_roots_; }

private:
    std::vector<std::string> search_roots_;
};

}  // namespace renderer
