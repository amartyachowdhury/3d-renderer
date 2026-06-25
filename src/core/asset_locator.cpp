#include "renderer/core/asset_locator.h"

#include <filesystem>
#include <fstream>

namespace renderer {

namespace {

std::string normalize_path(const std::filesystem::path& path) {
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        return canonical.string();
    }
    return path.lexically_normal().string();
}

bool file_exists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

std::string executable_directory() {
    std::error_code ec;
    const auto exe = std::filesystem::current_path(ec);
    if (ec) {
        return ".";
    }
    return exe.string();
}

}  // namespace

AssetLocator::AssetLocator() {
#ifdef RENDERER_SOURCE_DIR
    add_search_root(std::string(RENDERER_SOURCE_DIR) + "/assets");
#endif
#ifdef RENDERER_INSTALL_ASSETS_DIR
    add_search_root(RENDERER_INSTALL_ASSETS_DIR);
#endif

    const std::string exe_dir = executable_directory();
    add_search_root(exe_dir + "/assets");
    add_search_root(exe_dir + "/../share/3d-renderer/assets");
    add_search_root("assets");
}

void AssetLocator::add_search_root(const std::string& path) {
    if (path.empty()) {
        return;
    }

    const std::string normalized = normalize_path(std::filesystem::path(path));
    for (const auto& existing : search_roots_) {
        if (existing == normalized) {
            return;
        }
    }
    search_roots_.push_back(normalized);
}

std::string AssetLocator::resolve(const std::string& relative_path) const {
    if (relative_path.empty()) {
        return {};
    }

    if (file_exists(relative_path)) {
        return normalize_path(std::filesystem::path(relative_path));
    }

    for (const auto& root : search_roots_) {
        const std::filesystem::path candidate = std::filesystem::path(root) / relative_path;
        if (file_exists(candidate.string())) {
            return normalize_path(candidate);
        }
    }

    return relative_path;
}

bool AssetLocator::exists(const std::string& relative_path) const {
    const std::string resolved = resolve(relative_path);
    return file_exists(resolved);
}

std::string AssetLocator::user_output_path(const std::string& filename) const {
    std::error_code ec;
    const auto output_dir = std::filesystem::current_path(ec) / "output";
    if (!std::filesystem::exists(output_dir, ec)) {
        std::filesystem::create_directories(output_dir, ec);
    }
    return normalize_path(output_dir / filename);
}

}  // namespace renderer
