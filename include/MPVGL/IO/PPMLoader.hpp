#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Error/IOError.hpp"
#include "MPVGL/Graphics/Pixel.hpp"
#include "MPVGL/IO/ResourceBuffer.hpp"

namespace mpvgl {

struct PPMLoader {
    [[nodiscard]] static tl::expected<Pixel::Map<Pixel::RGB>, Error<IOError>>
    load(std::filesystem::path const& path) {
        auto bufferRes = io::ResourceBuffer::load(path);
        if (!bufferRes.has_value()) {
            return tl::unexpected{bufferRes.error()};
        }

        auto view = bufferRes->view();
        size_t offset = 0;

        auto skip_whitespace_and_comments = [&]() {
            while (offset < view.size()) {
                char c = static_cast<char>(view[offset]);
                if (std::isspace(c)) {
                    offset++;
                } else if (c == '#') {
                    while (offset < view.size() &&
                           static_cast<char>(view[offset]) != '\n') {
                        offset++;
                    }
                } else {
                    break;
                }
            }
        };

        auto read_string = [&]() -> std::string {
            skip_whitespace_and_comments();
            std::string result;
            while (offset < view.size()) {
                char c = static_cast<char>(view[offset]);
                if (std::isspace(c) || c == '#') break;
                result += c;
                offset++;
            }
            return result;
        };

        auto parse_uint = [](std::string const& s, uint32_t& out) -> bool {
            if (s.empty()) return false;
            uint32_t val = 0;
            for (char c : s) {
                if (!std::isdigit(c)) return false;
                val = val * 10 + (c - '0');
            }
            out = val;
            return true;
        };

        std::string magicNumber = read_string();
        if (magicNumber != "P6") {
            return tl::unexpected{
                Error{IOError::Unknown,
                      "Unsupported PPM format. Only binary P6 is supported: " +
                          path.string()}};
        }

        uint32_t width = 0, height = 0, maxVal = 0;
        if (!parse_uint(read_string(), width) ||
            !parse_uint(read_string(), height) ||
            !parse_uint(read_string(), maxVal)) {
            return tl::unexpected{
                Error{IOError::Unknown,
                      "Corrupted PPM header in file: " + path.string()}};
        }

        if (offset < view.size() &&
            std::isspace(static_cast<char>(view[offset]))) {
            offset++;
        }

        size_t expectedSize =
            static_cast<size_t>(width) * height * sizeof(Pixel::RGB);

        if (view.size() - offset < expectedSize) {
            return tl::unexpected{
                Error{IOError::EndOfFile,
                      "PPM file is truncated. Missing pixel data in file: " +
                          path.string()}};
        }

        std::vector<Pixel::RGB> pixels(width * height);
        std::memcpy(pixels.data(), view.data() + offset, expectedSize);

        return Pixel::Map<Pixel::RGB>(width, height, std::move(pixels));
    }
};

}  // namespace mpvgl
