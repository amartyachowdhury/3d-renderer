#include <algorithm>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

uint64_t fnv1a64(const uint8_t* data, size_t size) {
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool read_file_bytes(const std::string& path, std::vector<uint8_t>& bytes) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

uint64_t hash_ppm_pixels(const std::string& path) {
    std::vector<uint8_t> bytes;
    if (!read_file_bytes(path, bytes) || bytes.size() < 16) {
        return 0;
    }

    size_t pos = 0;
    int lines = 0;
    while (lines < 3 && pos < bytes.size()) {
        const size_t next = std::find(bytes.begin() + static_cast<std::ptrdiff_t>(pos), bytes.end(), static_cast<uint8_t>('\n')) - bytes.begin();
        pos = next + 1;
        ++lines;
    }

    if (pos >= bytes.size()) {
        return 0;
    }

    return fnv1a64(bytes.data() + pos, bytes.size() - pos);
}

bool read_expected_hash(const std::string& path, uint64_t& expected) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    in >> std::hex >> expected;
    return static_cast<bool>(in);
}

int run_command(const std::string& command) {
    return std::system(command.c_str());
}

int check_renderer(
    const char* name,
    const std::string& command,
    const std::string& output,
    const std::string& golden_path) {
    if (run_command(command) != 0) {
        std::cerr << "FAIL: command failed for " << name << '\n';
        std::cerr << "  command: " << command << '\n';
        return 1;
    }

    uint64_t expected = 0;
    if (!read_expected_hash(golden_path, expected)) {
        std::cerr << "FAIL: missing golden hash " << golden_path << '\n';
        return 1;
    }

    const uint64_t actual = hash_ppm_pixels(output);
    if (actual != expected) {
        std::cerr << "FAIL: " << name << " hash mismatch\n";
        std::cerr << "  expected: 0x" << std::hex << expected << '\n';
        std::cerr << "  actual:   0x" << actual << std::dec << '\n';
        return 1;
    }

    std::cout << name << " golden hash OK\n";
    return 0;
}

std::string quote_path(const std::string& path) {
    return '"' + path + '"';
}

std::string raytracer_golden_suffix() {
#if defined(__aarch64__) || defined(_M_ARM64)
    return ".arm64";
#else
    return ".x86_64";
#endif
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: test_golden RAYTRACER RASTERIZER RAYCASTER SOURCE_DIR\n";
        return 1;
    }

    const std::string raytracer = argv[1];
    const std::string rasterizer = argv[2];
    const std::string raycaster = argv[3];
    const std::string source_dir = argv[4];

    const std::string tmp_dir = std::filesystem::temp_directory_path().string();
    int failures = 0;

    failures += check_renderer(
        "raytracer",
        quote_path(raytracer) + " --scene " + quote_path(source_dir + "/assets/scenes/golden.scene") +
            " --width 32 --height 32 --samples 1 --threads 1 --max-depth 1 --dump-ppm --output " + quote_path(tmp_dir + "/golden_raytracer.ppm"),
        tmp_dir + "/golden_raytracer.ppm",
        source_dir + "/tests/golden/raytracer" + raytracer_golden_suffix() + ".hash");

    failures += check_renderer(
        "rasterizer",
        quote_path(rasterizer) + " --dump-ppm --output " + quote_path(tmp_dir + "/golden_rasterizer.ppm"),
        tmp_dir + "/golden_rasterizer.ppm",
        source_dir + "/tests/golden/rasterizer.hash");

    failures += check_renderer(
        "raycaster",
        quote_path(raycaster) + " --map " + quote_path(source_dir + "/assets/maps/level1.txt") +
            " --spawn " + quote_path(source_dir + "/assets/maps/level1.spawn") +
            " --dump-ppm --output " + quote_path(tmp_dir + "/golden_raycaster.ppm"),
        tmp_dir + "/golden_raycaster.ppm",
        source_dir + "/tests/golden/raycaster.hash");

    if (failures == 0) {
        std::cout << "All golden tests passed\n";
        return 0;
    }

    std::cerr << failures << " golden test(s) failed\n";
    return 1;
}
