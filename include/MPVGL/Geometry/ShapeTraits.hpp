#pragma once

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

struct Quad {};
struct Cube {};

template <typename ShapeTag>
struct ShapeTraits;

template <>
struct ShapeTraits<Quad> {
    static constexpr u32 vertexCount = 4;
    static constexpr u32 indexCount = 6;
    static constexpr bool is3D = false;
};

template <>
struct ShapeTraits<Cube> {
    static constexpr u32 vertexCount = 24;
    static constexpr u32 indexCount = 36;
    static constexpr bool is3D = true;
};

}  // namespace mpvgl
