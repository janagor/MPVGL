#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tinyobjloader/tiny_obj_loader.h>
#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Model.hpp"
#include "MPVGL/Core/Vulkan/Vertex.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/IO/ResourceBuffer.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

auto Model::loadFromFile(DeviceContext const& device, VkCommandPool commandPool,
                         VkQueue graphicsQueue, std::string const& filepath)
    -> tl::expected<Model, Error<EngineError>> {
    return io::ResourceBuffer::load(filepath)
        .map_error([](auto const& error) -> auto {
            return Error<EngineError>{EngineError::FileNotFound,
                                      error.message()};
        })
        .and_then([&](io::ResourceBuffer const& buffer)
                      -> tl::expected<Model, Error<EngineError>> {
            auto view = buffer.view();

            std::string_view const dataView(
                reinterpret_cast<char const*>(view.data()), view.size());

            auto reader_config = tinyobj::ObjReaderConfig{};
            auto reader = tinyobj::ObjReader{};

            if (!reader.ParseFromString(std::string(dataView), "")) {
                return tl::unexpected{
                    Error{EngineError::VulkanRuntimeError, reader.Error()}};
            }

            auto const& attrib = reader.GetAttrib();
            auto const& shapes = reader.GetShapes();

            auto vertices = std::vector<Vertex>{};
            auto indices = std::vector<u32>{};
            auto uniqueVertices = std::unordered_map<Vertex, u32>{};

            for (auto const& shape : shapes) {
                for (auto const& index : shape.mesh.indices) {
                    auto vertex = Vertex{
                        {attrib.vertices[(3 * index.vertex_index) + 0],
                         attrib.vertices[(3 * index.vertex_index) + 1],
                         attrib.vertices[(3 * index.vertex_index) + 2]},
                        {Color::Literal::White},
                        {attrib.texcoords[(2 * index.texcoord_index) + 0],
                         1.0F -
                             attrib.texcoords[(2 * index.texcoord_index) + 1]}};

                    if (!uniqueVertices.contains(vertex)) {
                        uniqueVertices[vertex] =
                            static_cast<u32>(vertices.size());
                        vertices.push_back(vertex);
                    }
                    indices.push_back(uniqueVertices.at(vertex));
                }
            }

            auto vBufferRes = Buffer::createFromData(
                device, commandPool, graphicsQueue, vertices,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            if (!vBufferRes) {
                return tl::unexpected{vBufferRes.error()};
            }

            auto iBufferRes = Buffer::createFromData(
                device, commandPool, graphicsQueue, indices,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            if (!iBufferRes) {
                return tl::unexpected{iBufferRes.error()};
            }

            return Model(std::move(vBufferRes.value()),
                         std::move(iBufferRes.value()),
                         static_cast<u32>(indices.size()));
        });
}

auto Model::create(DeviceContext const& device, VkCommandPool commandPool,
                   VkQueue graphicsQueue, std::vector<Vertex> const& vertices,
                   std::vector<u32> const& indices)
    -> tl::expected<Model, Error<EngineError>> {
    auto vBufferRes = Buffer::createFromData(
        device, commandPool, graphicsQueue, vertices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    if (!vBufferRes) {
        return tl::unexpected{vBufferRes.error()};
    }

    auto iBufferRes = Buffer::createFromData(
        device, commandPool, graphicsQueue, indices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    if (!iBufferRes) {
        return tl::unexpected{iBufferRes.error()};
    }

    return Model(std::move(vBufferRes.value()), std::move(iBufferRes.value()),
                 static_cast<u32>(indices.size()));
}

}  // namespace mpvgl::vlk
