#pragma once

#include <concepts>

namespace mpvgl {

template <typename T>
concept Arithmetic = std::integral<T> || std::f32ing_point<T>;

}
