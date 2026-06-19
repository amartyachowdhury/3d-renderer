#pragma once

#include <iostream>
#include <string>

namespace renderer {

inline void print_usage(const std::string& program, const std::string& text) {
    std::cout << "Usage: " << program << ' ' << text << '\n';
}

}  // namespace renderer
