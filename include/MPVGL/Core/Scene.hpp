#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "MPVGL/Core/Vulkan/Vertex.hpp"
#include "MPVGL/Geometry/Mesh.hpp"

namespace mpvgl {

struct SceneNode {
    Mesh<Vertex> mesh;
    glm::mat4 transform;
};

class Scene {
   public:
    void add(Mesh<Vertex> mesh, glm::mat4 transform = glm::mat4(1.0f)) {
        nodes.push_back({std::move(mesh), transform});
    }

    std::vector<SceneNode> nodes;
};

}  // namespace mpvgl
