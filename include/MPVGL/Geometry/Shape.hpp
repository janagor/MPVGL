#pragma once

#include "MPVGL/Core/Vulkan/Vertex.hpp"
#include "MPVGL/Geometry/Mesh.hpp"
#include "MPVGL/Graphics/Color.hpp"

namespace mpvgl {

template <typename ShapeTag>
struct Shape {
    [[nodiscard]] static Mesh<Vertex> generate(
        Color const& color = Color::Literal::White);
};

}  // namespace mpvgl

#include "MPVGL/Geometry/Shape.tpp"
