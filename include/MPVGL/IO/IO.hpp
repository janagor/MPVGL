#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Error/IOError.hpp"

namespace mpvgl::io {

class File {
   public:
    File(File const&) = delete;
    File& operator=(File const&) = delete;

    File(File&& other) noexcept = default;
    File& operator=(File&& other) noexcept = default;

    ~File() {
        if (m_stream.is_open()) {
            m_stream.close();
        }
    }

    [[nodiscard]] static tl::expected<File, Error<IOError>> open(
        std::filesystem::path const& path) {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);

        if (!stream.is_open()) {
            return tl::unexpected{
                Error{IOError::FileNotFound,
                      "Failed to open file: " + path.string()}};
        }

        return File(std::move(stream));
    }

    [[nodiscard]] tl::expected<std::vector<std::byte>, Error<IOError>>
    read_all() {
        if (!m_stream.is_open()) {
            return tl::unexpected{
                Error{IOError::Unknown, "Cannot read: File is not open"}};
        }

        std::streamsize size = m_stream.tellg();
        m_stream.seekg(0, std::ios::beg);

        std::vector<std::byte> buffer(size);
        if (m_stream.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return buffer;
        }

        return tl::unexpected{
            Error{IOError::Unknown, "Failed to read file content"}};
    }

    [[nodiscard]] std::ifstream& get_stream() noexcept {
        if (m_stream.tellg() != 0) {
            m_stream.seekg(0, std::ios::beg);
        }
        return m_stream;
    }

    [[nodiscard]] std::size_t size() {
        auto current_pos = m_stream.tellg();
        m_stream.seekg(0, std::ios::end);
        std::size_t size = m_stream.tellg();
        m_stream.seekg(current_pos);
        return size;
    }

   private:
    explicit File(std::ifstream&& stream) : m_stream(std::move(stream)) {}

    std::ifstream m_stream;
};

}  // namespace mpvgl::io
