#pragma once

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

struct Color {
    constexpr Color(u8 red, u8 green, u8 blue) noexcept
        : m_red(static_cast<f32>(red) / 255.0),
          m_green(static_cast<f32>(green) / 255.0),
          m_blue(static_cast<f32>(blue) / 255.0) {}

    struct literal;

    f32 m_red;
    f32 m_green;
    f32 m_blue;
};

struct Color::literal {
    static constexpr Color Red = Color(255, 0, 0);
    static constexpr Color Green = Color(0, 255, 0);
    static constexpr Color Blue = Color(0, 0, 255);
    static constexpr Color Black = Color(0, 0, 0);
    static constexpr Color White = Color(255, 255, 255);
};

}  // namespace mpvgl
