#include <algorithm>
namespace mpvgl {
namespace Pixel {

namespace {
constexpr uint8_t rgbToGray(auto const& r, auto const& g, auto const& b) {
    return static_cast<uint8_t>(0.2126 * r + 0.7152 * g + 0.0722 * b);
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
    if constexpr (std::is_same_v<ToT, RGBA>)
        return RGBA(this->m_g, this->m_g, this->m_g, 255);
    if constexpr (std::is_same_v<ToT, GrayA>) return GrayA(this->m_g, 255);
}

template <PixelLike ToT>
constexpr ToT GrayA::convertTo() const {
    if constexpr (std::is_same_v<ToT, RGB>)
        return RGB(this->m_g, this->m_g, this->m_g);
    if constexpr (std::is_same_v<ToT, RGBA>)
        return RGBA(this->m_g, this->m_g, this->m_g, this->m_a);
    if constexpr (std::is_same_v<ToT, Gray>) return Gray(this->m_g);
}

template <PixelLike ToT>
constexpr ToT RGB::convertTo() const {
    if constexpr (std::is_same_v<ToT, RGBA>)
        return RGBA(this->m_r, this->m_g, this->m_b, 255);
    if constexpr (std::is_same_v<ToT, Gray>)
        return Gray(rgbToGray(this->m_r, this->m_g, this->m_b));
    if constexpr (std::is_same_v<ToT, GrayA>)
        return GrayA(rgbToGray(this->m_r, this->m_g, this->m_b), 255);
}

template <PixelLike ToT>
constexpr ToT RGBA::convertTo() const {
    if constexpr (std::is_same_v<ToT, RGB>)
        return RGB(this->m_r, this->m_g, this->m_b);
    if constexpr (std::is_same_v<ToT, Gray>)
        return Gray(rgbToGray(this->m_r, this->m_g, this->m_b));
    if constexpr (std::is_same_v<ToT, GrayA>)
        return GrayA(rgbToGray(this->m_r, this->m_g, this->m_b), 255);
}

template <PixelLike PixelT>
template <PixelLike OtherT>
constexpr Map<PixelT>::operator Map<OtherT>() const {
    std::vector<OtherT> newData{};
    newData.reserve(m_data.size());
    std::transform(m_data.begin(), m_data.end(), std::back_inserter(newData),
                   [](auto const& px) { return static_cast<OtherT>(px); });

    return Map<OtherT>{m_width, m_height, newData};
}

}  // namespace Pixel
}  // namespace mpvgl
