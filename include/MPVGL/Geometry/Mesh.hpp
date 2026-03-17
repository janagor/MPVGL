#pragma once

#include <cstdint>
#include <vector>

namespace mpvgl {

template <typename VertexT, typename IndexT = std::uint32_t>
struct Mesh {
    std::vector<VertexT> vertices;
    std::vector<IndexT> indices;
};

}  // namespace mpvgl
