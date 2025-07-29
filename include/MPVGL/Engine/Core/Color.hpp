#pragma once

#include <cstdint>

namespace mpvgl {

struct Color {
  constexpr Color(uint8_t red, uint8_t green, uint8_t blue) noexcept
      : m_red(static_cast<float>(red) / 255.0),
        m_green(static_cast<float>(green) / 255.0),
        m_blue(static_cast<float>(blue) / 255.0) {}

  struct literal;

  float m_red;
  float m_green;
  float m_blue;
};

struct Color::literal {
  static constexpr Color Red = Color(255, 0, 0);
  static constexpr Color Green = Color(0, 255, 0);
  static constexpr Color Blue = Color(0, 0, 255);
  static constexpr Color Black = Color(0, 0, 0);
  static constexpr Color White = Color(255, 255, 255);
};

} // namespace mpvgl
