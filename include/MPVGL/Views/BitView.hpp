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

#include <cstddef>
#include <ranges>
#include <type_traits>

namespace mpvgl::views {

struct BigEndianTag {};
struct LittleEndianTag {};

template <std::ranges::view View, typename EndianTag = LittleEndianTag>
class BitView : public std::ranges::view_interface<BitView<View, EndianTag>> {
   private:
    View m_base = View();

    class Sentinel;

    class Iterator {
       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = bool;

        Iterator() = default;
        constexpr explicit Iterator(std::ranges::iterator_t<View> it);

        constexpr bool operator*() const;
        constexpr Iterator& operator++();
        constexpr Iterator operator++(int);
        constexpr bool operator==(Iterator const& other) const;
        constexpr std::ranges::iterator_t<View> base() const;

       private:
        std::ranges::iterator_t<View> m_current;
        int m_bit_offset = 0;
    };

    class Sentinel {
       public:
        Sentinel() = default;
        constexpr explicit Sentinel(std::ranges::sentinel_t<View> end);
        constexpr bool operator==(Iterator const& it) const;

       private:
        std::ranges::sentinel_t<View> m_end;
    };

   public:
    BitView() = default;
    constexpr explicit BitView(View base);

    constexpr auto begin();
    constexpr auto end();
};

template <typename EndianTag>
struct ToBitsFn {
    template <std::ranges::viewable_range R>
    constexpr auto operator()(R&& r) const;
};

template <std::ranges::viewable_range R, typename EndianTag>
constexpr auto operator|(R&& r, ToBitsFn<EndianTag> fn);

inline constexpr ToBitsFn<BigEndianTag> ToBitsBigEndian;
inline constexpr ToBitsFn<LittleEndianTag> ToBitsLittleEndian;
inline constexpr auto ToBits = ToBitsBigEndian;

}  // namespace mpvgl::views

#include "BitView.tpp"
