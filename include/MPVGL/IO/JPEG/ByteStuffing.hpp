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
#include <iterator>
#include <ranges>
#include <span>

#include "MPVGL/Utility/BitMask.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::jpeg {

class ByteStuffingIterator {
   public:
    using difference_type = std::ptrdiff_t;
    using value_type = u8;
    using pointer = u8*;
    using reference = u8&;
    using const_reference = u8 const&;
    using iterator_category = std::input_iterator_tag;

    ByteStuffingIterator() = default;
    ByteStuffingIterator(std::span<std::byte const>::iterator& begin,
                         std::span<std::byte const>::iterator end)
        : m_base_iter(&begin), m_end(end) {
        advance();
    }

    auto operator*() const -> const_reference { return m_current; }

    auto operator++() -> ByteStuffingIterator& {
        advance();
        return *this;
    }
    auto operator++(int /*i*/) -> void { advance(); }
    auto operator==(std::default_sentinel_t /*sentinel*/) const -> bool {
        return m_is_at_end;
    }

   private:
    void advance() noexcept {
        if (nullptr == m_base_iter || *m_base_iter == m_end) {
            m_is_at_end = true;
            return;
        }
        auto byte = static_cast<u8>(**m_base_iter);
        ++(*m_base_iter);

        if (mask::u8set == byte && *m_base_iter != m_end) {
            auto peek = static_cast<u8>(**m_base_iter);
            if (peek == 0) {
                ++(*m_base_iter);
            }
        }
        m_current = byte;
        m_is_at_end = false;
    }

    std::span<std::byte const>::iterator* m_base_iter = nullptr;
    std::span<std::byte const>::iterator m_end;
    value_type m_current = 0;
    bool m_is_at_end = true;
};

class ByteStuffingView : public std::ranges::view_interface<ByteStuffingView> {
   public:
    ByteStuffingView(std::span<std::byte const>::iterator& iter,
                     std::span<std::byte const>::iterator end)
        : m_iter(&iter), m_end(end) {}

    [[nodiscard]] auto begin() const -> ByteStuffingIterator {
        return {*m_iter, m_end};
    }
    [[nodiscard]] auto end() { return std::default_sentinel; }

   private:
    std::span<std::byte const>::iterator* m_iter;
    std::span<std::byte const>::iterator m_end;
};

}  // namespace mpvgl::jpeg
