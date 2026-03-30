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
#include <cstdint>
#include <iterator>
#include <string>

namespace mpvgl {

namespace detail {

template <typename T>
[[nodiscard]] constexpr T adjust_endianness(T value) noexcept {
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (std::integral<T>) {
        return std::byteswap(value);
    } else if constexpr (std::floating_point<T> && sizeof(T) == 4) {
        return std::bit_cast<T>(std::byteswap(std::bit_cast<uint32_t>(value)));
    } else if constexpr (std::floating_point<T> && sizeof(T) == 8) {
        return std::bit_cast<T>(std::byteswap(std::bit_cast<uint64_t>(value)));
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
[[nodiscard]] constexpr T readType(Iter& iterator) {
    T value;
    auto* dst = reinterpret_cast<std::byte*>(std::addressof(value));

    for (std::size_t i = 0; i < sizeof(T); ++i) {
        dst[i] = static_cast<std::byte>(*iterator);
        ++iterator;
    }

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
[[nodiscard]] constexpr T peekType(Iter iterator) {
    // Kopia iteratora jest przekazana przez wartość (Iter iterator),
    // więc readType nie przesunie oryginalnego!
    return readType<T, Endian>(iterator);
}

/**
 * Reads a fixed-point number and converts it to a floating-point
 * representation.
 */
template <std::endian Endian = std::endian::little,
          std::integral BaseType = int32_t,
          std::floating_point FloatType = float, std::size_t Shift = 16,
          std::input_iterator Iter>
[[nodiscard]] constexpr FloatType readFixed(Iter& iterator) {
    BaseType rawValue = readType<BaseType, Endian>(iterator);
    return static_cast<FloatType>(rawValue) /
           static_cast<FloatType>(1ULL << Shift);
}

/**
 * Peeks a fixed-point number.
 */
template <std::endian Endian = std::endian::little,
          std::integral BaseType = int32_t,
          std::floating_point FloatType = float, std::size_t Shift = 16,
          std::input_iterator Iter>
[[nodiscard]] constexpr FloatType peekFixed(Iter iterator) {
    return readFixed<Endian, BaseType, FloatType, Shift>(iterator);
}

/**
 * Reads n-chars from the iterator. Advances iterator by length.
 */
template <std::input_iterator Iter>
[[nodiscard]] inline std::string readNChars(std::size_t length,
                                            Iter& iterator) {
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
[[nodiscard]] inline std::string peekNChars(std::size_t length, Iter iterator) {
    return readNChars(length, iterator);
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr T readNBits(std::size_t length, Iter& iter) {
    T result = 0;
    for (std::size_t i = 0; i < length; ++i) {
        result = (result << 1) | static_cast<T>(*iter ? 1 : 0);
        ++iter;
    }
    return result;
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr T peekNBits(std::size_t length, Iter iter) {
    return readNBits<T>(length, iter);
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr T readRNBits(std::size_t length, Iter& iter) {
    T result = 0;
    for (std::size_t i = 0; i < length; ++i) {
        result |= (static_cast<T>(*iter ? 1 : 0) << i);
        ++iter;
    }
    return result;
}

template <std::integral T, std::input_iterator Iter>
[[nodiscard]] constexpr T peekRNBits(std::size_t length, Iter iter) {
    return readRNBits<T>(length, iter);
}

}  // namespace mpvgl
