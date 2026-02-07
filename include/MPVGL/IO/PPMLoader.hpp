#pragma once

#include <stdint.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "MPVGL/Graphics/Pixel.hpp"
#include "MPVGL/IO/Loader.hpp"

namespace mpvgl {

struct PPMLoader {
  static Pixel::Map<Pixel::RGB> createPixelMap(std::string const &data) {
    char ch;
    std::string magicNumber;
    uint8_t current, r, g, b;
    uint32_t width, height, maxVal;

    std::istringstream iss(data);
    iss >> magicNumber >> width >> height >> maxVal;

    iss.ignore(1);

    std::vector<Pixel::RGB> pixels(width * height);
    iss.read(reinterpret_cast<char *>(pixels.data()),
             pixels.size() * sizeof(Pixel::RGB));

    return Pixel::Map<Pixel::RGB>(width, height, pixels);
  }
  static Pixel::Map<Pixel::RGB> load(std::filesystem::path const &path) {
    auto data = Loader::load(path);
    return createPixelMap(data);
  }
};

}  // namespace mpvgl
