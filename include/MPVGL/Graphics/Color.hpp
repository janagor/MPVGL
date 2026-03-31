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

    [[nodiscard]] constexpr auto red() const noexcept -> f64 { return m_red; }
    [[nodiscard]] constexpr auto green() const noexcept -> f64 {
        return m_green;
    }
    [[nodiscard]] constexpr auto blue() const noexcept -> f64 { return m_blue; }

   private:
    f64 m_red;
    f64 m_green;
    f64 m_blue;
};

struct Color::Literal {
    static constexpr Color Red = Color{{.r = 255, .g = 0, .b = 0}};
    static constexpr Color Green = Color{{.r = 0, .g = 255, .b = 0}};
    static constexpr Color Blue = Color{{.r = 0, .g = 0, .b = 255}};
    static constexpr Color Black = Color{{.r = 0, .g = 0, .b = 0}};
    static constexpr Color White = Color{{.r = 255, .g = 255, .b = 255}};
};

}  // namespace mpvgl
