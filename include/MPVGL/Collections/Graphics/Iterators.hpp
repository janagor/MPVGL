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

#include <iterator>
#include <ranges>

#include "MPVGL/Utility/MemoryUtils.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

template <typename T>
class StridedIterator {
   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    StridedIterator() = default;
    StridedIterator(T* ptr, std::size_t strideBytes)
        : m_ptr(ptr), m_stride(strideBytes) {}

    [[nodiscard]] auto operator*() const noexcept -> reference {
        return *m_ptr;
    }
    [[nodiscard]] auto operator->() const noexcept -> pointer { return m_ptr; }

    auto operator++() noexcept -> StridedIterator& {
        m_ptr = utils::byte_offset<T>(m_ptr, m_stride);
        return *this;
    }
    auto operator++(int) noexcept -> StridedIterator {
        auto temp = *this;
        ++(*this);
        return temp;
    }
    auto operator--() noexcept -> StridedIterator& {
        m_ptr = utils::byte_offset<T>(m_ptr,
                                      -static_cast<std::ptrdiff_t>(m_stride));
        return *this;
    }
    auto operator--(int) noexcept -> StridedIterator {
        auto temp = *this;
        --(*this);
        return temp;
    }

    auto operator+=(difference_type diff) noexcept -> StridedIterator& {
        m_ptr = utils::byte_offset<T>(m_ptr, diff * m_stride);
        return *this;
    }
    auto operator-=(difference_type diff) noexcept -> StridedIterator& {
        m_ptr = utils::byte_offset<T>(m_ptr, -diff * m_stride);
        return *this;
    }

    [[nodiscard]] friend auto operator+(StridedIterator iter,
                                        difference_type diff) noexcept
        -> StridedIterator {
        iter += diff;
        return iter;
    }
    [[nodiscard]] friend auto operator+(difference_type diff,
                                        StridedIterator iter) noexcept
        -> StridedIterator {
        iter += diff;
        return iter;
    }
    [[nodiscard]] friend auto operator-(StridedIterator iter,
                                        difference_type diff) noexcept
        -> StridedIterator {
        iter -= diff;
        return iter;
    }
    [[nodiscard]] friend auto operator-(StridedIterator const& lhs,
                                        StridedIterator const& rhs) noexcept
        -> difference_type {
        auto const* lhsBytes = reinterpret_cast<std::byte const*>(lhs.m_ptr);
        auto const* rhsBytes = reinterpret_cast<std::byte const*>(rhs.m_ptr);
        return (lhsBytes - rhsBytes) / lhs.m_stride;
    }

    [[nodiscard]] auto operator[](difference_type diff) const noexcept
        -> reference {
        return *(*this + diff);
    }
    [[nodiscard]] auto operator==(StridedIterator const& other) const noexcept
        -> bool {
        return m_ptr == other.m_ptr;
    }
    [[nodiscard]] auto operator!=(StridedIterator const& other) const noexcept
        -> bool {
        return m_ptr != other.m_ptr;
    }
    [[nodiscard]] auto operator<(StridedIterator const& other) const noexcept
        -> bool {
        return m_ptr < other.m_ptr;
    }
    [[nodiscard]] auto operator>(StridedIterator const& other) const noexcept
        -> bool {
        return m_ptr > other.m_ptr;
    }
    [[nodiscard]] auto operator<=(StridedIterator const& other) const noexcept
        -> bool {
        return m_ptr <= other.m_ptr;
    }
    [[nodiscard]] auto operator>=(StridedIterator const& other) const noexcept
        -> bool {
        return m_ptr >= other.m_ptr;
    }

   private:
    T* m_ptr{nullptr};
    std::size_t m_stride{0};
};

template <typename T>
class ImageIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    ImageIterator() = default;
    ImageIterator(u32 width, T* rowPtr, std::size_t stride)
        : m_rowPtr(rowPtr), m_width(width), m_stride(stride) {}

    [[nodiscard]] auto operator*() const noexcept -> reference {
        return std::span<T>{m_rowPtr, m_width}[m_x];
    }
    [[nodiscard]] auto operator->() const noexcept -> pointer {
        return std::span<T>{m_rowPtr, m_width}[m_x];
    }

    auto operator++() noexcept -> ImageIterator& {
        ++m_x;
        if (m_x == m_width) {
            m_x = 0;
            // Przeskakujemy na początek następnego wiersza uwzględniając
            // padding
            m_rowPtr = utils::byte_offset<T>(m_rowPtr, m_stride);
        }
        return *this;
    }

    auto operator++(int) noexcept -> ImageIterator {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    [[nodiscard]] auto operator==(ImageIterator const& other) const noexcept
        -> bool {
        return m_rowPtr == other.m_rowPtr && m_x == other.m_x;
    }
    [[nodiscard]] auto operator!=(ImageIterator const& other) const noexcept
        -> bool {
        return !(*this == other);
    }

   private:
    T* m_rowPtr{nullptr};
    u32 m_x{0};
    u32 m_width{0};
    std::size_t m_stride{0};
};

}  // namespace mpvgl
