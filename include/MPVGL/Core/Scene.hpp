#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "MPVGL/Core/Vulkan/Vertex.hpp"
#include "MPVGL/Geometry/Mesh.hpp"

namespace mpvgl {

struct SceneNode {
    Mesh<Vertex> mesh;
    glm::mat4 transform;
    std::string texturePath;
};

class Scene {
   public:
    void add(Mesh<Vertex> mesh, glm::mat4 transform = glm::mat4(1.0f),
             std::string texturePath = "textures/default.ppm") {
        nodes.emplace_back(std::move(mesh), transform, std::move(texturePath));
    }

    std::vector<SceneNode> nodes;
};

}  // namespace mpvgl
