/**
 *  Your Project
 *
 *  Copyright (C) 2026 Jan Aleksander Górski
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would
 *     be appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source
 *     distribution.
 */
#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

#include <tl/expected.hpp>

#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Error/IOError.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::io {

class BinaryReader {
   public:
    explicit BinaryReader(std::span<std::byte const> data) noexcept
        : m_data(data), m_offset(0) {}

    explicit BinaryReader(std::span<char const> data) noexcept
        : m_data(std::as_bytes(data)), m_offset(0) {}

    explicit BinaryReader(std::span<u8 const> data) noexcept
        : m_data(std::as_bytes(data)), m_offset(0) {}

    [[nodiscard]] bool isEOF() const noexcept {
        return m_offset >= m_data.size();
    }

    [[nodiscard]] std::size_t remaining() const noexcept {
        return m_data.size() - m_offset;
    }

    [[nodiscard]] tl::expected<void, Error<IOError>> skip(std::size_t bytes) {
        if (bytes > remaining()) {
            return tl::unexpected{
                Error{IOError::EndOfFile,
                      "BinaryReader: Buffer boundary exceeded during skip()"}};
        }
        m_offset += bytes;
        return {};
    }

    template <typename T, std::endian Endianness = std::endian::little>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] tl::expected<T, Error<IOError>> peek() const {
        if (sizeof(T) > remaining()) {
            return tl::unexpected{
                Error{IOError::EndOfFile,
                      "BinaryReader: Buffer boundary exceeded during peek()"}};
        }

        T value;
        std::memcpy(&value, m_data.data() + m_offset, sizeof(T));
        return adjustEndianness<T, Endianness>(value);
    }

    template <typename T, std::endian Endianness = std::endian::little>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] tl::expected<T, Error<IOError>> read() {
        auto result = peek<T, Endianness>();
        if (result.has_value()) {
            m_offset += sizeof(T);
        }
        return result;
    }

    [[nodiscard]] tl::expected<std::span<std::byte const>, Error<IOError>>
    readSpan(std::size_t size) {
        if (size > remaining()) {
            return tl::unexpected{Error{IOError::EndOfFile,
                                        "BinaryReader: Buffer boundary "
                                        "exceeded during readSpan()"}};
        }
        auto result = m_data.subspan(m_offset, size);
        m_offset += size;
        return result;
    }

    [[nodiscard]] tl::expected<std::string_view, Error<IOError>> readStringView(
        std::size_t length) {
        return readSpan(length).transform([](std::span<std::byte const> bytes) {
            return std::string_view(reinterpret_cast<char const*>(bytes.data()),
                                    bytes.size());
        });
    }

   private:
    std::span<std::byte const> m_data;
    std::size_t m_offset;

    template <typename T, std::endian Endianness>
    [[nodiscard]] static constexpr T adjustEndianness(T value) noexcept {
        if constexpr (Endianness == std::endian::native) return value;
        if constexpr (sizeof(T) == 1) return value;
        if constexpr (std::integral<T>) return std::byteswap(value);
        if constexpr (std::floating_point<T> && sizeof(T) == 4)
            return std::bit_cast<T>(std::byteswap(std::bit_cast<u32>(value)));
        if constexpr (std::floating_point<T> && sizeof(T) == 8)
            return std::bit_cast<T>(std::byteswap(std::bit_cast<u64>(value)));
    }
};

}  // namespace mpvgl::io
