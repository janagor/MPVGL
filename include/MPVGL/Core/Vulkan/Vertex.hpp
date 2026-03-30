#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include <array>
#include <cstddef>
#include <functional>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Graphics/Color.hpp"

namespace mpvgl {

struct Vertex {
    Vertex(glm::vec3 pos, Color color, glm::vec2 texCoord)
        : m_pos(pos),
          m_color({color.red(), color.green(), color.blue()}),
          m_texCoord(texCoord) {}

    [[nodiscard]] glm::vec3 const& pos() const noexcept { return m_pos; }
    [[nodiscard]] glm::vec3 const& color() const noexcept { return m_color; }
    [[nodiscard]] glm::vec2 const& texCoord() const noexcept {
        return m_texCoord;
    }

    static VkVertexInputBindingDescription getBindingDescription() noexcept {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static constexpr std::array<VkVertexInputAttributeDescription, 3>
    getAttributeDescriptions() noexcept {
        std::array<VkVertexInputAttributeDescription, 3>
            attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, m_pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, m_color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, m_texCoord);

        return attributeDescriptions;
    }
    bool operator==(Vertex const& other) const {
        return m_pos == other.pos() && m_color == other.color() &&
               m_texCoord == other.texCoord();
    }

   private:
    glm::vec3 m_pos;
    glm::vec3 m_color;
    glm::vec2 m_texCoord;
};

}  // namespace mpvgl

namespace std {

template <>
struct hash<mpvgl::Vertex> {
    size_t operator()(mpvgl::Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos()) ^
                 (hash<glm::vec3>()(vertex.color()) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.texCoord()) << 1);
    }
};

}  // namespace std
