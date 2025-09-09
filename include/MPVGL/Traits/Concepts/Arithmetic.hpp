#pragma once

#include <concepts>

namespace mpvgl {

template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

}
