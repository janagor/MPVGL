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
#include <ranges>

namespace mpvgl::views {

struct BigEndianTag {};
struct LittleEndianTag {};

template <std::ranges::view View, typename T,
          typename EndianTag = LittleEndianTag>
    requires std::is_trivially_copyable_v<T>
class AsTypeView
    : public std::ranges::view_interface<AsTypeView<View, T, EndianTag>> {
   private:
    View m_base = View();

    class Sentinel;

    class Iterator {
       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = T;

        Iterator() = default;
        constexpr explicit Iterator(std::ranges::iterator_t<View> it)
            : m_current(std::move(it)) {}

        constexpr T operator*() const {
            T value;
            std::memcpy(&value, std::to_address(m_current), sizeof(T));
            return adjust_endianness(value);
        }

        constexpr Iterator& operator++() {
            std::advance(m_current, sizeof(T));
            return *this;
        }

        constexpr Iterator operator++(int) {
            Iterator tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr bool operator==(Iterator const& other) const {
            return m_current == other.m_current;
        }

        constexpr std::ranges::iterator_t<View> base() const {
            return m_current;
        }

       private:
        std::ranges::iterator_t<View> m_current;

        static constexpr T adjust_endianness(T value) noexcept {
            if constexpr (sizeof(T) == 1) {
                return value;
            } else {
                constexpr bool is_big_tag =
                    std::is_same_v<EndianTag, BigEndianTag>;
                constexpr bool is_native_big =
                    (std::endian::native == std::endian::big);

                if constexpr (is_big_tag != is_native_big) {
                    if constexpr (std::integral<T>) {
                        return std::byteswap(value);
                    } else if constexpr (std::f32ing_point<T> &&
                                         sizeof(T) == 4) {
                        return std::bit_cast<T>(
                            std::byteswap(std::bit_cast<u32>(value)));
                    } else if constexpr (std::f32ing_point<T> &&
                                         sizeof(T) == 8) {
                        return std::bit_cast<T>(
                            std::byteswap(std::bit_cast<ui64>(value)));
                    }
                }
                return value;
            }
        }
    };

    class Sentinel {
       public:
        Sentinel() = default;
        constexpr explicit Sentinel(std::ranges::sentinel_t<View> end)
            : m_end(std::move(end)) {}

        constexpr bool operator==(Iterator const& it) const {
            return std::ranges::distance(it.base(), m_end) <
                   static_cast<std::ptrdiff_t>(sizeof(T));
        }

       private:
        std::ranges::sentinel_t<View> m_end;
    };

   public:
    AsTypeView() = default;
    constexpr explicit AsTypeView(View base) : m_base(std::move(base)) {}

    constexpr auto begin() { return Iterator(std::ranges::begin(m_base)); }
    constexpr auto end() { return Sentinel(std::ranges::end(m_base)); }
};

template <typename T, typename EndianTag>
struct AsTypeFn {
    template <std::ranges::viewable_range R>
    constexpr auto operator()(R&& r) const {
        return AsTypeView<std::views::all_t<R>, T, EndianTag>(
            std::views::all(std::forward<R>(r)));
    }
};

template <std::ranges::viewable_range R, typename T, typename EndianTag>
constexpr auto operator|(R&& r, AsTypeFn<T, EndianTag> fn) {
    return fn(std::forward<R>(r));
}

template <typename T>
inline constexpr AsTypeFn<T, LittleEndianTag> as_little_endian;

template <typename T>
inline constexpr AsTypeFn<T, BigEndianTag> as_big_endian;

}  // namespace mpvgl::views
