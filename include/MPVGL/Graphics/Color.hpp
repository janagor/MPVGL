#pragma once

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

struct Color {
    static constexpr f64 MAX_COLOR_VALUE = 255.0;
    struct RGB {
        u8 r;
        u8 g;
        u8 b;
    };
    constexpr Color(RGB rgb) noexcept
        : m_red(static_cast<f64>(rgb.r) / MAX_COLOR_VALUE),
          m_green(static_cast<f64>(rgb.g) / MAX_COLOR_VALUE),
          m_blue(static_cast<f64>(rgb.b) / MAX_COLOR_VALUE) {}

    struct Literal;

    [[nodiscard]] constexpr f64 red() const noexcept { return m_red; }
    [[nodiscard]] constexpr f64 green() const noexcept { return m_green; }
    [[nodiscard]] constexpr f64 blue() const noexcept { return m_blue; }

   private:
    f64 m_red;
    f64 m_green;
    f64 m_blue;
};

struct Color::Literal {
    static constexpr Color Red = Color{{255, 0, 0}};
    static constexpr Color Green = Color{{0, 255, 0}};
    static constexpr Color Blue = Color{{0, 0, 255}};
    static constexpr Color Black = Color{{0, 0, 0}};
    static constexpr Color White = Color{{255, 255, 255}};
};

}  // namespace mpvgl
