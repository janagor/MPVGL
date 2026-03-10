#pragma once

#include <fstream>
#include <iostream>
#include <vector>

#include <tl/expected.hpp>

#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Error/IOError.hpp"

namespace mpvgl {

tl::expected<std::vector<char>, Error<IOError>> readFile(
    std::string const &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return tl::unexpected{
            Error{IOError::FileNotFound, "Failed to open a File"}};
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    file.close();

    return buffer;
}
}  // namespace mpvgl
