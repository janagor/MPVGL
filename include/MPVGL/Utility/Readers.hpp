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
#include <iterator>
#include <string>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

namespace detail {

template <typename T>
[[nodiscard]] constexpr auto adjust_endianness(T value) noexcept -> T {
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (std::integral<T>) {
        return std::byteswap(value);
    } else if constexpr (std::floating_point<T> && sizeof(T) == sizeof(u32)) {
        return std::bit_cast<T>(std::byteswap(std::bit_cast<u32>(value)));
    } else if constexpr (std::floating_point<T> && sizeof(T) == sizeof(u64)) {
        return std::bit_cast<T>(std::byteswap(std::bit_cast<u64>(value)));
    }
    return value;
}

}  // namespace detail

/**
 * Reads a trivially copyable type from the given iterator.
 * Advances iterator by the size of the type.
 */
template <typename T, std::endian Endian = std::endian::little,
          std::input_iterator Iter>
    requires std::is_trivially_copyable_v<T>
[[nodiscard]] constexpr auto readType(Iter& iterator) -> T {
    std::array<std::byte, sizeof(T)> buffer{};

    for (std::size_t i{}; i < sizeof(T); ++i) {
        buffer.at(i) = static_cast<std::byte>(*iterator++);
    }

    T value = std::bit_cast<T>(buffer);

    if constexpr (Endian != std::endian::native) {
        return detail::adjust_endianness(value);
    }

    return value;
}

/**
 * Peeks a trivially copyable type from the given iterator without advancing the
 * original.
 */
template <typename T, std::endian Endian = std::endian::little,
          std::input_iterator Iter>
    requires std::is_trivially_copyable_v<T>
[[nodiscard]] constexpr auto peekType(Iter iterator) -> T {
    return readType<T, Endian>(iterator);
}

/**
 * Reads a fixed-point number and converts it to a floating-point
 * representation.
 */
template <std::endian Endian = std::endian::little,
          std::integral BaseType = i32, std::floating_point FloatType = f32,
          std::size_t Shift = UINT16_WIDTH, std::input_iterator Iter>
[[nodiscard]] constexpr auto readFixed(Iter& iterator) -> FloatType {
    BaseType rawValue = readType<BaseType, Endian>(iterator);
    return static_cast<FloatType>(rawValue) /
           static_cast<FloatType>(1ULL << Shift);
}

/**
 * Peeks a fixed-point number.
 */
template <std::endian Endian = std::endian::little,
          std::integral BaseType = i32, std::floating_point FloatType = f32,
          std::size_t Shift = UINT16_WIDTH, std::input_iterator Iter>
[[nodiscard]] constexpr auto peekFixed(Iter iterator) -> FloatType {
    return readFixed<Endian, BaseType, FloatType, Shift>(iterator);
}

/**
 * Reads n-chars from the iterator. Advances iterator by length.
 */
template <std::input_iterator Iter>
[[nodiscard]] inline auto readNChars(std::size_t length, Iter& iterator)
    -> std::string {
    std::string result;
    result.reserve(length);  // Pre-alokacja dla wydajności

    for (std::size_t i = 0; i < length; ++i) {
        result.push_back(static_cast<char>(*iterator));
        ++iterator;
    }
    return result;
}

/**
 * Peeks n-chars from the iterator.
 */
template <std::input_iterator Iter>
[[nodiscard]] inline auto peekNChars(std::size_t length, Iter iterator)
    -> std::string {
    return readNChars(length, iterator);
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr auto readNBits(std::size_t length, Iter& iter) -> T {
    T result = 0;
    for (std::size_t i = 0; i < length; ++i) {
        result = (result << 1) | static_cast<T>(*iter ? 1 : 0);
        ++iter;
    }
    return result;
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr auto peekNBits(std::size_t length, Iter iter) -> T {
    return readNBits<T>(length, iter);
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr auto readRNBits(std::size_t length, Iter& iter) -> T {
    T result = 0;
    for (std::size_t i = 0; i < length; ++i) {
        result |= (static_cast<T>(*iter ? 1 : 0) << i);
        ++iter;
    }
    return result;
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr auto peekRNBits(std::size_t length, Iter iter) -> T {
    return readRNBits<T>(length, iter);
}

}  // namespace mpvgl
