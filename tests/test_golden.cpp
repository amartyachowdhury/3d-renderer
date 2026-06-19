#include <algorithm>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#endif

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

std::string path_string(const std::filesystem::path& path) {
    return path.string();
}

int run_process(const std::vector<std::string>& args) {
    if (args.empty()) {
        return -1;
    }

#ifdef _WIN32
    std::vector<std::string> native_args;
    native_args.reserve(args.size());
    for (const auto& arg : args) {
        native_args.push_back(std::filesystem::path(arg).string());
    }

    std::vector<const char*> argv;
    argv.reserve(native_args.size() + 1);
    for (const auto& arg : native_args) {
        argv.push_back(arg.c_str());
    }
    argv.push_back(nullptr);

    return _spawnv(_P_WAIT, native_args[0].c_str(), argv.data());
#else
    std::ostringstream command;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            command << ' ';
        }
        command << '"' << args[i] << '"';
    }
    return std::system(command.str().c_str());
#endif
}

std::string format_command(const std::vector<std::string>& args) {
    std::ostringstream command;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            command << ' ';
        }
        command << '"' << args[i] << '"';
    }
    return command.str();
}

int check_renderer(
    const char* name,
    const std::vector<std::string>& args,
    const std::string& output,
    const std::string& golden_path) {
    if (run_process(args) != 0) {
        std::cerr << "FAIL: command failed for " << name << '\n';
        std::cerr << "  command: " << format_command(args) << '\n';
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

std::string raytracer_golden_suffix() {
#if defined(__aarch64__) || defined(_M_ARM64)
    return ".arm64";
#else
    return ".x86_64";
#endif
}

std::string raytracer_golden_path(const std::filesystem::path& source_dir) {
#if defined(_WIN32)
    return path_string(source_dir / "tests/golden/raytracer.windows.hash");
#else
    return path_string(source_dir / ("tests/golden/raytracer" + raytracer_golden_suffix() + ".hash"));
#endif
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: test_golden RAYTRACER RASTERIZER RAYCASTER SOURCE_DIR\n";
        return 1;
    }

    const std::filesystem::path source_dir = argv[4];
    const std::filesystem::path tmp_dir = std::filesystem::temp_directory_path();
    const std::string raytracer_out = path_string(tmp_dir / "golden_raytracer.ppm");
    const std::string rasterizer_out = path_string(tmp_dir / "golden_rasterizer.ppm");
    const std::string raycaster_out = path_string(tmp_dir / "golden_raycaster.ppm");

    int failures = 0;

    failures += check_renderer(
        "raytracer",
        {
            argv[1],
            "--scene",
            path_string(source_dir / "assets/scenes/golden.scene"),
            "--width",
            "32",
            "--height",
            "32",
            "--samples",
            "1",
            "--threads",
            "1",
            "--max-depth",
            "1",
            "--dump-ppm",
            "--output",
            raytracer_out,
        },
        raytracer_out,
        raytracer_golden_path(source_dir));

    failures += check_renderer(
        "rasterizer",
        {
            argv[2],
            "--dump-ppm",
            "--output",
            rasterizer_out,
        },
        rasterizer_out,
        path_string(source_dir / "tests/golden/rasterizer.hash"));

    failures += check_renderer(
        "raycaster",
        {
            argv[3],
            "--map",
            path_string(source_dir / "assets/maps/level1.txt"),
            "--spawn",
            path_string(source_dir / "assets/maps/level1.spawn"),
            "--dump-ppm",
            "--output",
            raycaster_out,
        },
        raycaster_out,
        path_string(source_dir / "tests/golden/raycaster.hash"));

    if (failures == 0) {
        std::cout << "All golden tests passed\n";
        return 0;
    }

    std::cerr << failures << " golden test(s) failed\n";
    return 1;
}
