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

namespace mpvgl::views {

template <std::ranges::view View, typename EndianTag>
constexpr BitView<View, EndianTag>::BitView(View base)
    : m_base(std::move(base)) {}

template <std::ranges::view View, typename EndianTag>
constexpr auto BitView<View, EndianTag>::begin() {
    return Iterator(std::ranges::begin(m_base));
}

template <std::ranges::view View, typename EndianTag>
constexpr auto BitView<View, EndianTag>::end() {
    return Sentinel(std::ranges::end(m_base));
}

template <std::ranges::view View, typename EndianTag>
constexpr BitView<View, EndianTag>::Iterator::Iterator(
    std::ranges::iterator_t<View> it)
    : m_current(std::move(it)), m_bit_offset(0) {}

template <std::ranges::view View, typename EndianTag>
constexpr bool BitView<View, EndianTag>::Iterator::operator*() const {
    if constexpr (std::is_same_v<EndianTag, BigEndianTag>) {
        return (*m_current >> (7 - m_bit_offset)) & 1;
    } else if constexpr (std::is_same_v<EndianTag, LittleEndianTag>) {
        return (*m_current >> m_bit_offset) & 1;
    } else {
        static_assert(std::is_same_v<EndianTag, BigEndianTag> ||
                          std::is_same_v<EndianTag, LittleEndianTag>,
                      "Nieobslugiwany tag kolejnosci bitow! Uzyj BigEndianTag "
                      "lub LittleEndianTag.");
        return false;
    }
}

template <std::ranges::view View, typename EndianTag>
constexpr typename BitView<View, EndianTag>::Iterator&
BitView<View, EndianTag>::Iterator::operator++() {
    if (++m_bit_offset == 8) {
        m_bit_offset = 0;
        ++m_current;
    }
    return *this;
}

template <std::ranges::view View, typename EndianTag>
constexpr typename BitView<View, EndianTag>::Iterator
BitView<View, EndianTag>::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++*this;
    return tmp;
}

template <std::ranges::view View, typename EndianTag>
constexpr bool BitView<View, EndianTag>::Iterator::operator==(
    Iterator const& other) const {
    return m_current == other.m_current && m_bit_offset == other.m_bit_offset;
}

template <std::ranges::view View, typename EndianTag>
constexpr std::ranges::iterator_t<View>
BitView<View, EndianTag>::Iterator::base() const {
    return m_current;
}

template <std::ranges::view View, typename EndianTag>
constexpr BitView<View, EndianTag>::Sentinel::Sentinel(
    std::ranges::sentinel_t<View> end)
    : m_end(std::move(end)) {}

template <std::ranges::view View, typename EndianTag>
constexpr bool BitView<View, EndianTag>::Sentinel::operator==(
    Iterator const& it) const {
    return it.base() == m_end;
}

template <typename EndianTag>
template <std::ranges::viewable_range R>
constexpr auto ToBitsFn<EndianTag>::operator()(R&& r) const {
    return BitView<std::views::all_t<R>, EndianTag>(
        std::views::all(std::forward<R>(r)));
}

template <std::ranges::viewable_range R, typename EndianTag>
constexpr auto operator|(R&& r, ToBitsFn<EndianTag> fn) {
    return fn(std::forward<R>(r));
}

}  // namespace mpvgl::views
