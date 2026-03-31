#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "MPVGL/Core/Vulkan/Vertex.hpp"
#include "MPVGL/Geometry/Mesh.hpp"

namespace mpvgl {

struct SceneNode {
    Mesh<Vertex> mesh{};
    glm::mat4 transform{};
    std::string texturePath;
};

class Scene {
   public:
    void add(Mesh<Vertex> mesh, glm::mat4 transform = glm::mat4(1.0F),
             std::string texturePath = "textures/default.ppm") {
        m_nodes.emplace_back(std::move(mesh), transform,
                             std::move(texturePath));
    }

    [[nodiscard]] auto nodes() const noexcept -> std::vector<SceneNode> const& {
        return m_nodes;
    }
    [[nodiscard]] auto nodes() noexcept -> std::vector<SceneNode>& {
        return m_nodes;
    }

   private:
    std::vector<SceneNode> m_nodes;
};

}  // namespace mpvgl
