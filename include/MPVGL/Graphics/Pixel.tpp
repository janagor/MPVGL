#include <algorithm>
#include <utility>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

namespace {
constexpr u8 rgbToGray(u8 r, u8 g, u8 b) noexcept {
    return static_cast<u8>(0.2126f * r + 0.7152f * g + 0.0722f * b);
}
}  // namespace

template <typename Self, typename... Pixels>
template <typename TargetPixel>
    requires(!std::is_same_v<TargetPixel, Self>)
constexpr PixelConversions<Self, std::tuple<Pixels...>>::operator TargetPixel()
    const {
    return static_cast<Self const*>(this)->template convertTo<TargetPixel>();
}

template <PixelLike ToT>
constexpr ToT Gray::convertTo() const {
    if constexpr (std::is_same_v<ToT, RGB>)
        return RGB(this->m_g, this->m_g, this->m_g);
    else if constexpr (std::is_same_v<ToT, RGBA>)
        return RGBA(this->m_g, this->m_g, this->m_g, 255);
    else if constexpr (std::is_same_v<ToT, GrayA>)
        return GrayA(this->m_g, 255);
    else
        static_assert(sizeof(ToT) == 0, "Unsupported pixel conversion!");
}

template <PixelLike ToT>
constexpr ToT GrayA::convertTo() const {
    if constexpr (std::is_same_v<ToT, RGB>)
        return RGB(this->m_g, this->m_g, this->m_g);
    else if constexpr (std::is_same_v<ToT, RGBA>)
        return RGBA(this->m_g, this->m_g, this->m_g, this->m_a);
    else if constexpr (std::is_same_v<ToT, Gray>)
        return Gray(this->m_g);
    else
        static_assert(sizeof(ToT) == 0, "Unsupported pixel conversion!");
}

template <PixelLike ToT>
constexpr ToT RGB::convertTo() const {
    if constexpr (std::is_same_v<ToT, RGBA>)
        return RGBA(this->m_r, this->m_g, this->m_b, 255);
    else if constexpr (std::is_same_v<ToT, Gray>)
        return Gray(rgbToGray(this->m_r, this->m_g, this->m_b));
    else if constexpr (std::is_same_v<ToT, GrayA>)
        return GrayA(rgbToGray(this->m_r, this->m_g, this->m_b), 255);
    else
        static_assert(sizeof(ToT) == 0, "Unsupported pixel conversion!");
}

template <PixelLike ToT>
constexpr ToT RGBA::convertTo() const {
    if constexpr (std::is_same_v<ToT, RGB>)
        return RGB(this->m_r, this->m_g, this->m_b);
    else if constexpr (std::is_same_v<ToT, Gray>)
        return Gray(rgbToGray(this->m_r, this->m_g, this->m_b));
    else if constexpr (std::is_same_v<ToT, GrayA>)
        return GrayA(rgbToGray(this->m_r, this->m_g, this->m_b), 255);
    else
        static_assert(sizeof(ToT) == 0, "Unsupported pixel conversion!");
}

template <PixelLike PixelT>
template <PixelLike OtherT>
constexpr Map<PixelT>::operator Map<OtherT>() const {
    std::vector<OtherT> newData{};
    newData.reserve(m_data.size());

    std::transform(m_data.begin(), m_data.end(), std::back_inserter(newData),
                   [](auto const& px) { return static_cast<OtherT>(px); });

    return Map<OtherT>{m_width, m_height, std::move(newData)};
}

}  // namespace mpvgl
