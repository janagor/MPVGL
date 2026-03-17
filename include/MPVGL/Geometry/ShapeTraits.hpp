#pragma once

#include <cstdint>

namespace mpvgl {

struct Quad {};
struct Cube {};

template <typename ShapeTag>
struct ShapeTraits;

template <>
struct ShapeTraits<Quad> {
    static constexpr std::uint32_t vertexCount = 4;
    static constexpr std::uint32_t indexCount = 6;
    static constexpr bool is3D = false;
};

template <>
struct ShapeTraits<Cube> {
    static constexpr std::uint32_t vertexCount = 24;
    static constexpr std::uint32_t indexCount = 36;
    static constexpr bool is3D = true;
};

}  // namespace mpvgl
