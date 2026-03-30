#pragma once

#include <concepts>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

template <typename Derived>
struct Base;
struct RGB;
struct RGBA;
struct Gray;
struct GrayA;

using PixelTypes = std::tuple<Gray, RGB, RGBA, GrayA>;

template <typename T>
concept PixelLike = std::derived_from<T, Base<T>>;

template <typename Self, typename PixelList>
struct PixelConversions;

template <typename Self, typename... Pixels>
struct PixelConversions<Self, std::tuple<Pixels...>> {
    template <typename TargetPixel>
        requires(!std::is_same_v<TargetPixel, Self>)
    constexpr operator TargetPixel() const;
};

using SubPixel = u8;

#pragma pack(push, 1)

template <typename Derived>
struct Base {
    constexpr friend bool operator==(Derived const& lhs,
                                     Derived const& rhs) noexcept {
        return lhs.data() == rhs.data();
    };
    constexpr friend bool operator!=(Derived const& lhs,
                                     Derived const& rhs) noexcept {
        return !(lhs == rhs);
    };
    constexpr auto data() const noexcept;

    static constexpr std::size_t channels() noexcept {
        return Derived::CHANNELS;
    }
};

struct RGB : Base<RGB>, PixelConversions<RGB, PixelTypes> {
   public:
    friend struct PixelConversions<RGB, PixelTypes>;
    static constexpr std::size_t CHANNELS = 3;

    RGB() : m_r(0), m_g(0), m_b(0) {}
    RGB(SubPixel const& r, SubPixel const& g, SubPixel const& b)
        : m_r(r), m_g(g), m_b(b) {}

    constexpr auto data() const noexcept { return std::tie(m_r, m_g, m_b); };

   private:
    template <PixelLike ToT>
    constexpr ToT convertTo() const;

   private:
    SubPixel m_r, m_g, m_b;
};

struct RGBA : Base<RGBA>, PixelConversions<RGBA, PixelTypes> {
   public:
    friend struct PixelConversions<RGBA, PixelTypes>;
    static constexpr std::size_t CHANNELS = 4;

    RGBA() : m_r(0), m_g(0), m_b(0), m_a(0) {}
    RGBA(SubPixel const& r, SubPixel const& g, SubPixel const& b,
         SubPixel const& a)
        : m_r(r), m_g(g), m_b(b), m_a(a) {}

    constexpr auto data() const noexcept {
        return std::tie(m_r, m_g, m_b, m_a);
    };

   private:
    template <PixelLike ToT>
    constexpr ToT convertTo() const;

   private:
    SubPixel m_r, m_g, m_b, m_a;
};

struct Gray : Base<Gray>, PixelConversions<Gray, PixelTypes> {
    friend struct PixelConversions<Gray, PixelTypes>;

   public:
    static constexpr std::size_t CHANNELS = 1;

    Gray() : m_g(0) {}
    Gray(SubPixel const& g) : m_g(g) {}

    constexpr auto data() const noexcept { return std::tie(m_g); };

   private:
    template <PixelLike ToT>
    constexpr ToT convertTo() const;

   private:
    SubPixel m_g;
};

struct GrayA : Base<GrayA>, PixelConversions<GrayA, PixelTypes> {
    friend struct PixelConversions<GrayA, PixelTypes>;

   public:
    static constexpr std::size_t CHANNELS = 2;

    GrayA() : m_g(0), m_a(0) {}
    GrayA(SubPixel const& g, SubPixel const& a) : m_g(g), m_a(a) {}

    constexpr auto data() const noexcept { return std::tie(m_g, m_a); };

   private:
    template <PixelLike ToT>
    constexpr ToT convertTo() const;

   private:
    SubPixel m_g, m_a;
};

#pragma pack(pop)

template <PixelLike PixelT>
struct Map {
    explicit Map(u32 width, u32 height, std::vector<PixelT> data)
        : m_width(width), m_height(height), m_data(std::move(data)) {}

    template <PixelLike OtherT>
    constexpr operator Map<OtherT>() const;

    u8* dataPtr() noexcept { return reinterpret_cast<u8*>(m_data.data()); }
    u8 const* dataPtr() const noexcept {
        return reinterpret_cast<u8 const*>(m_data.data());
    }

    std::vector<PixelT>& data() noexcept { return m_data; }
    std::vector<PixelT> const& data() const noexcept { return m_data; }

    u32 height() const noexcept { return m_height; }
    u32 width() const noexcept { return m_width; }
    std::size_t channels() const noexcept { return PixelT::channels(); }

   private:
    u32 m_width, m_height;
    std::vector<PixelT> m_data;
};

}  // namespace mpvgl

#include "MPVGL/Graphics/Pixel.tpp"
