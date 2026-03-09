#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tinyobjloader/tiny_obj_loader.h>
#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Model.hpp"
#include "MPVGL/Core/Vulkan/Vertex.hpp"

namespace mpvgl::vlk {

tl::expected<Model, Error> Model::loadFromFile(DeviceContext const& device,
                                               VkCommandPool commandPool,
                                               VkQueue graphicsQueue,
                                               std::string const& filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          filepath.c_str())) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to load Model: " + filepath}};
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (auto const& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            auto vertex =
                Vertex{{attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]},
                       {255, 255, 255},
                       {attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices.at(vertex));
        };
    };

    auto vBufferRes = Buffer::createFromData(
        device, commandPool, graphicsQueue, vertices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    if (!vBufferRes) return tl::unexpected(vBufferRes.error());

    auto iBufferRes = Buffer::createFromData(
        device, commandPool, graphicsQueue, indices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    if (!iBufferRes) return tl::unexpected(iBufferRes.error());

    return Model(std::move(vBufferRes.value()), std::move(iBufferRes.value()),
                 static_cast<uint32_t>(indices.size()));
}

}  // namespace mpvgl::vlk
