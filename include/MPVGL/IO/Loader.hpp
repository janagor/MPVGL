#pragma once

#include <stdint.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace mpvgl {
struct Loader {
  inline static std::string load(std::filesystem::path const &path) {
    if (auto file = std::ifstream(path, std::ios::binary | std::ios::ate)) {
      std::streamsize size = file.tellg();
      file.seekg(0, std::ios::beg);
      std::string result(size, '\0');
      if (file.read(&result[0], size)) {
        return result;
      }
      throw std::runtime_error("Cannot read file");
    }
    throw std::runtime_error("Cannot open file");
  }
};
}  // namespace mpvgl
