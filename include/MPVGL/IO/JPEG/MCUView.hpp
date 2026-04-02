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

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <vector>

#include "MPVGL/Collections/Graphics/ColorSpace.hpp"
#include "MPVGL/Graphics/ColorConversion.hpp"
#include "MPVGL/Graphics/Extent.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::jpeg {

struct Component {
    u8 horizontalSampling : 4 = 0;
    u8 verticalSampling : 4 = 0;
    u8 id : 2 = 0;
    u8 quantizationTableNumber : 2 = 0;
    u8 dcHuffmanTableId : 2 = 0;
    u8 acHuffmanTableId : 2 = 0;
};

class MCUView : public std::ranges::view_interface<MCUView> {
    static constexpr size_t BLOCK_SIZE = 64;
    static constexpr size_t BLOCK_DIM = 8;
    using Block = std::array<i32, BLOCK_SIZE>;

   public:
    class Iterator {
       public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = PixelRGB8;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = PixelRGB8;

        Iterator() = default;
        Iterator(MCUView const* view, std::size_t index)
            : m_view(view), m_index(index) {}

        [[nodiscard]] auto operator*() const noexcept -> reference {
            return m_view->operator[](m_index);
        }
        [[nodiscard]] auto operator[](difference_type n) const noexcept
            -> reference {
            return *(*this + n);
        }
        auto operator++() noexcept -> Iterator& {
            ++m_index;
            return *this;
        }
        auto operator++(int) noexcept -> Iterator {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        auto operator--() noexcept -> Iterator& {
            --m_index;
            return *this;
        }
        auto operator--(int) noexcept -> Iterator {
            auto tmp = *this;
            --(*this);
            return tmp;
        }
        auto operator+=(difference_type diff) noexcept -> Iterator& {
            m_index += diff;
            return *this;
        }
        auto operator-=(difference_type diff) noexcept -> Iterator& {
            m_index -= diff;
            return *this;
        }

        [[nodiscard]] friend auto operator+(Iterator iter,
                                            difference_type diff) noexcept
            -> Iterator {
            iter += diff;
            return iter;
        }
        [[nodiscard]] friend auto operator+(difference_type diff,
                                            Iterator iter) noexcept
            -> Iterator {
            iter += diff;
            return iter;
        }
        [[nodiscard]] friend auto operator-(Iterator iter,
                                            difference_type diff) noexcept
            -> Iterator {
            iter -= diff;
            return iter;
        }
        [[nodiscard]] friend auto operator-(Iterator const& lhs,
                                            Iterator const& rhs) noexcept
            -> difference_type {
            return static_cast<difference_type>(lhs.m_index) -
                   static_cast<difference_type>(rhs.m_index);
        }

        [[nodiscard]] auto operator==(Iterator const& other) const noexcept
            -> bool {
            return m_index == other.m_index;
        }
        [[nodiscard]] auto operator<=>(Iterator const& other) const noexcept =
            default;

       private:
        MCUView const* m_view{nullptr};
        std::size_t m_index{0};
    };

    MCUView(std::vector<Component> const& components,
            std::vector<std::vector<Block>> const& blocks, Extent2D maxSampling)
        : m_components(components),
          m_blocks(blocks),
          m_maxSampling(maxSampling),
          m_width(maxSampling.width * BLOCK_DIM),
          m_height(maxSampling.height * BLOCK_DIM) {}

    [[nodiscard]] auto begin() const noexcept -> Iterator { return {this, 0}; }
    [[nodiscard]] auto end() const noexcept -> Iterator {
        return {this, size()};
    }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return static_cast<size_t>(m_width) * static_cast<size_t>(m_height);
    }
    [[nodiscard]] auto width() const noexcept -> u32 { return m_width; }
    [[nodiscard]] auto height() const noexcept -> u32 { return m_height; }

    [[nodiscard]] auto operator[](std::size_t index) const noexcept
        -> PixelRGB8 {
        static constexpr u8 DEFAULT_CHROMINANCE = 128;
        static constexpr u8 DEFAULT_LUMINANCE = 0;
        auto indices = Indices2D{.x = index % m_width, .y = index / m_width};
        u8 yVal = 0;
        u8 cbVal = DEFAULT_CHROMINANCE;
        u8 crVal = DEFAULT_CHROMINANCE;

        for (size_t compIdx = 0; compIdx < m_components.get().size();
             ++compIdx) {
            auto const& comp = m_components.get()[compIdx];
            auto compX =
                (indices.x * comp.horizontalSampling) / m_maxSampling.width;
            auto compY =
                (indices.y * comp.verticalSampling) / m_maxSampling.height;
            auto blockH = compX / BLOCK_DIM;
            auto blockV = compY / BLOCK_DIM;
            auto pixelInBlockX = compX % BLOCK_DIM;
            auto pixelInBlockY = compY % BLOCK_DIM;
            auto blockIdx = (blockV * comp.horizontalSampling) + blockH;
            auto val = m_blocks.get().at(compIdx).at(blockIdx).at(
                (pixelInBlockY * BLOCK_DIM) + pixelInBlockX);

            if (1 == comp.id) {
                yVal = val;
            } else if (2 == comp.id) {
                cbVal = val;
            } else if (3 == comp.id) {
                crVal = val;
            }
        }

        PixelRGB8 pixel{};
        if (m_components.get().size() >= 3) {
            convert<PixelYCbCr8, PixelRGB8>(
                PixelYCbCr8{{.y = yVal, .cb = cbVal, .cr = crVal}}, pixel);
        } else {
            auto clampGray =
                static_cast<u8>(std::clamp(yVal, ZERO<u8>, U8_MAX<u8>));
            pixel.r() = clampGray;
            pixel.g() = clampGray;
            pixel.b() = clampGray;
        }
        return pixel;
    }

   private:
    std::reference_wrapper<std::vector<Component> const> m_components;
    std::reference_wrapper<std::vector<std::vector<Block>> const> m_blocks;
    Extent2D m_maxSampling;
    u32 m_width;
    u32 m_height;
};

}  // namespace mpvgl::jpeg
