#pragma once

#include <vector>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

template <typename VertexT, typename IndexT = u32>
struct Mesh {
    std::vector<VertexT> vertices;
    std::vector<IndexT> indices;
};

}  // namespace mpvgl
