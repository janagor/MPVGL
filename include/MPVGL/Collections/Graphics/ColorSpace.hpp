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

#include <concepts>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

#include "MPVGL/Collections/Graphics/Iterators.hpp"
#include "MPVGL/Graphics/Extent.hpp"
#include "MPVGL/Graphics/Indices.hpp"
#include "MPVGL/Utility/MemoryUtils.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

template <typename T>
struct RGB {
    T r{}, g{}, b{};
};
template <typename T>
struct RGBA {
    T r{}, g{}, b{}, a{};
};
template <typename T>
struct YCbCr {
    T y{}, cb{}, cr{};
};

struct ColorSpaceRGB {
    static constexpr std::size_t channels = 3;
};
struct ColorSpaceRGBA {
    static constexpr std::size_t channels = 4;
};
struct ColorSpaceYCbCr {
    static constexpr std::size_t channels = 3;
};

template <typename T>
concept ChannelType = std::is_arithmetic_v<T>;

template <typename T>
concept ColorSpaceType = requires {
    { T::channels } -> std::convertible_to<std::size_t>;
};

template <class CSpace, ChannelType T = u8>
struct Pixel {
    using Channel = T;
    static constexpr std::size_t CHANNELS = CSpace::channels;

    constexpr Pixel() noexcept = default;

    explicit constexpr Pixel(RGB<Channel> args) noexcept
        requires(std::same_as<CSpace, ColorSpaceRGB>)
        : m_data{args.r, args.g, args.b} {}

    explicit constexpr Pixel(RGBA<Channel> args) noexcept
        requires(std::same_as<CSpace, ColorSpaceRGBA>)
        : m_data{args.r, args.g, args.b, args.a} {}

    explicit constexpr Pixel(YCbCr<Channel> args) noexcept
        requires(std::same_as<CSpace, ColorSpaceYCbCr>)
        : m_data{args.y, args.cb, args.cr} {}

    // RGB and RGBA
    template <class Self>
    [[nodiscard]] constexpr auto r(this Self&& self) -> auto&&
        requires((CHANNELS >= 1) && (std::same_as<CSpace, ColorSpaceRGB> ||
                                     std::same_as<CSpace, ColorSpaceRGBA>))
    {
        return std::forward<Self>(self).m_data[0];
    }
    template <class Self>
    [[nodiscard]] constexpr auto g(this Self&& self) -> auto&&
        requires((CHANNELS >= 2) && (std::same_as<CSpace, ColorSpaceRGB> ||
                                     std::same_as<CSpace, ColorSpaceRGBA>))
    {
        return std::forward<Self>(self).m_data[1];
    }
    template <class Self>
    [[nodiscard]] constexpr auto b(this Self&& self) -> auto&&

        requires((CHANNELS >= 3) && (std::same_as<CSpace, ColorSpaceRGB> ||
                                     std::same_as<CSpace, ColorSpaceRGBA>))
    {
        return std::forward<Self>(self).m_data[2];
    }
    template <class Self>
    [[nodiscard]] constexpr auto a(this Self&& self) -> auto&&
        requires((CHANNELS >= 4) && (std::same_as<CSpace, ColorSpaceRGBA>))
    {
        return std::forward<Self>(self).m_data[3];
    }

    // YCbCr
    template <class Self>
    [[nodiscard]] constexpr auto y(this Self&& self) -> auto&&
        requires((CHANNELS >= 1) && (std::same_as<CSpace, ColorSpaceYCbCr>))
    {
        return std::forward<Self>(self).m_data[0];
    }
    template <class Self>
    [[nodiscard]] constexpr auto cb(this Self&& self) -> auto&&
        requires((CHANNELS >= 2) && (std::same_as<CSpace, ColorSpaceYCbCr>))
    {
        return std::forward<Self>(self).m_data[1];
    }
    template <class Self>
    [[nodiscard]] constexpr auto cr(this Self&& self) -> auto&&
        requires((CHANNELS >= 3) && (std::same_as<CSpace, ColorSpaceYCbCr>))
    {
        return std::forward<Self>(self).m_data[2];
    }

   private:
    template <class Self>
    [[nodiscard]] constexpr auto operator[](this Self&& self,
                                            std::size_t idx) noexcept
        -> auto&& {
        return std::forward<Self>(self).m_data[idx];
    }

    std::array<T, CHANNELS> m_data{};
};

using PixelRGB8 = Pixel<ColorSpaceRGB, u8>;
using PixelRGBA8 = Pixel<ColorSpaceRGBA, u8>;
using PixelYCbCr8 = Pixel<ColorSpaceYCbCr, u8>;

template <typename PixelT>
class ImageView {
   public:
    using PixelType = PixelT;
    using Channel = typename std::remove_const_t<PixelT>::Channel;
    using LineView = std::ranges::subrange<StridedIterator<PixelT>>;

    ImageView() = default;

    ImageView(PixelT* data, Extent2D const& extent, std::size_t stride)
        : m_data(data),
          m_width(extent.width),
          m_height(extent.height),
          m_stride(stride) {}

    [[nodiscard]] auto width() const noexcept -> u32 { return m_width; }
    [[nodiscard]] auto height() const noexcept -> u32 { return m_height; }
    [[nodiscard]] auto stride() const noexcept -> std::size_t {
        return m_stride;
    }

    [[nodiscard]] auto operator()(Indices2D const& indices) noexcept
        -> PixelT& {
        auto* rowPtr = utils::byte_offset<PixelT>(m_data, indices.y * m_stride);
        std::span<PixelT> row{rowPtr, m_width};
        return row[indices.x];
    }

    [[nodiscard]] auto operator()(Indices2D const& indices) const noexcept
        -> PixelT const& {
        auto const* rowPtr =
            utils::byte_offset<PixelT const>(m_data, indices.y * m_stride);
        std::span<PixelT const> row{rowPtr, m_width};
        return row[indices.x];
    }
    [[nodiscard]] auto begin() const noexcept -> ImageIterator<PixelT> {
        return ImageIterator<PixelT>(m_width, m_data, m_stride);
    }

    [[nodiscard]] auto end() const noexcept -> ImageIterator<PixelT> {
        auto* endRow = utils::byte_offset<PixelT>(m_data, m_height * m_stride);
        return ImageIterator<PixelT>(m_width, endRow, m_stride);
    }

    [[nodiscard]] auto row(u32 yIndex) const noexcept -> LineView {
        auto* firstPixel = &(*this)({0, yIndex});
        auto* endPixel =
            utils::byte_offset<PixelT>(firstPixel, m_width * sizeof(PixelT));

        return std::ranges::subrange(
            StridedIterator<PixelT>(firstPixel, sizeof(PixelT)),
            StridedIterator<PixelT>(endPixel, sizeof(PixelT)));
    }

    [[nodiscard]] auto column(u32 xIndex) const noexcept -> LineView {
        auto* firstPixel = &(*this)({xIndex, 0});
        auto* endPixel =
            utils::byte_offset<PixelT>(firstPixel, m_height * m_stride);

        return std::ranges::subrange(
            StridedIterator<PixelT>(firstPixel, m_stride),
            StridedIterator<PixelT>(endPixel, m_stride));
    }

    [[nodiscard]] auto subView(Indices2D const& indices,
                               Extent2D const& extent) const -> ImageView {
        return ImageView{&(*this)(indices), extent, m_stride};
    }

   private:
    PixelT* m_data{nullptr};
    u32 m_width{0};
    u32 m_height{0};
    std::size_t m_stride{0};
};

template <typename PixelT>
class Image {
   public:
    Image() = default;
    Image(Image const&) = delete;
    auto operator=(Image const&) -> Image& = delete;
    auto operator=(Image&& other) noexcept -> Image& {
        if (other != *this) {
            m_storage = std::exchange(other.m_storage, {});
            m_width = std::exchange(other.m_width, 0);
            m_height = std::exchange(other.m_height, 0);
            m_stride = std::exchange(other.m_stride, 0);
        }
        return *this;
    }
    Image(Image&& other) noexcept
        : m_storage(std::exchange(other.m_storage, {})),
          m_width(std::exchange(other.m_width, 0)),
          m_height(std::exchange(other.m_height, 0)),
          m_stride(std::exchange(other.m_stride, 0)) {}
    ~Image() = default;

    explicit Image(Extent2D const& extent) { allocate(extent); }

    auto allocate(Extent2D const& extent) -> void {
        m_width = extent.width;
        m_height = extent.height;
        m_stride = extent.width * sizeof(PixelT);
        m_storage.resize(extent.width * extent.height);
    }

    [[nodiscard]] auto view() noexcept -> ImageView<PixelT> {
        return ImageView<PixelT>(m_storage.data(), {m_width, m_height},
                                 m_stride);
    }

    [[nodiscard]] auto view() const noexcept -> ImageView<PixelT const> {
        return ImageView<PixelT const>(m_storage.data(), {m_width, m_height},
                                       m_stride);
    }

    [[nodiscard]] auto bytes() const noexcept -> std::span<std::byte const> {
        return std::as_bytes(std::span{m_storage});
    }

    [[nodiscard]] auto bytes() noexcept -> std::span<std::byte> {
        return std::as_writable_bytes(std::span{m_storage});
    }

   private:
    std::vector<PixelT> m_storage;
    u32 m_width{0};
    u32 m_height{0};
    std::size_t m_stride{0};
};

using DynamicImage = std::variant<Image<PixelRGB8>, Image<PixelRGBA8>
                                  // Image<PixelGray8>, Image<PixelRGB32F>
                                  >;

}  // namespace mpvgl
