#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

namespace mpvgl {
namespace Pixel {

#pragma pack(push, 1)
// clang-format off
template <typename Derived>
struct Base; struct RGB; struct RGBA; struct Gray; struct GrayA;
// clang-format on
using PixelTypes = std::tuple<Gray, RGB, RGBA, GrayA>;
template <typename T>
concept PixelLike = std::derived_from<T, Base<T>>;

template <typename Self, typename PixelList>
struct PixelConversions;

template <typename PixelType>
struct PixelTraits {
    static constexpr std::size_t channels() noexcept {
        if constexpr (std::is_same_v<PixelType, RGB>)
            return 3;
        else if constexpr (std::is_same_v<PixelType, RGBA>)
            return 4;
        else if constexpr (std::is_same_v<PixelType, Gray>)
            return 1;
        else if constexpr (std::is_same_v<PixelType, GrayA>)
            return 2;
        else
            static_assert(sizeof(PixelType) == 0, "Nieznany typ piksela");
    }
};

template <typename Self, typename... Pixels>
struct PixelConversions<Self, std::tuple<Pixels...>> {
    template <typename TargetPixel>
        requires(!std::is_same_v<TargetPixel, Self>)
    constexpr operator TargetPixel() const;
};
using SubPixel = uint8_t;

template <typename Derived>
struct Base : PixelTraits<Derived> {
    constexpr friend bool operator==(Derived const& lhs,
                                     Derived const& rhs) noexcept {
        return lhs.data() == rhs.data();
    };
    constexpr friend bool operator!=(Derived const& lhs,
                                     Derived const& rhs) noexcept {
        return !(lhs == rhs);
    };
    constexpr auto data() const noexcept;
};

struct RGB : Base<RGB>, PixelConversions<RGB, PixelTypes> {
   public:
    friend struct PixelConversions<RGB, PixelTypes>;
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
    GrayA() : m_g(0), m_a(0) {}
    GrayA(SubPixel const& g, SubPixel const& a) : m_g(g), m_a(a) {}

    constexpr auto data() const noexcept { return std::tie(m_g, m_a); };

   private:
    template <PixelLike ToT>
    constexpr ToT convertTo() const;

   private:
    SubPixel m_g, m_a;
};

template <PixelLike PixelT>
struct Map {
    explicit Map(uint32_t width, uint32_t height,
                 std::vector<PixelT> const& data)
        : m_width(width), m_height(height), m_data(data) {}

    template <PixelLike OtherT>
    constexpr operator Map<OtherT>() const;
    uint8_t* dataPtr() noexcept {
        return reinterpret_cast<uint8_t*>(m_data.data());
    }
    const uint8_t* dataPtr() const noexcept {
        return reinterpret_cast<const uint8_t*>(m_data.data());
    }
    std::vector<PixelT>& data() noexcept { return m_data; }
    const std::vector<PixelT>& data() const noexcept { return m_data; }
    uint32_t height() const noexcept { return m_height; }
    uint32_t width() const noexcept { return m_width; }
    std::size_t channels() const noexcept { return PixelT::channels(); }

   private:
    uint32_t m_width, m_height;
    std::vector<PixelT> m_data;
};

#pragma pack(pop)

}  // namespace Pixel
}  // namespace mpvgl

#include "MPVGL/Graphics/Pixel.tpp"
