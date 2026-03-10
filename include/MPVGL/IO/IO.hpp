#pragma once

#include <vector>

#include <tl/expected.hpp>

tl::expected<std::vector<char>, Error> readFile(std::string const &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to open a File"};
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    file.close();

    return buffer;
}
