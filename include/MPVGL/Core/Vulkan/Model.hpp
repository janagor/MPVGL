#pragma once

#include <string>
#include <utility>

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Vertex.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

class Model {
   public:
    Model() = default;
    Model(Buffer vertexBuffer, Buffer indexBuffer, u32 indexCount)
        : m_vertexBuffer(std::move(vertexBuffer)),
          m_indexBuffer(std::move(indexBuffer)),
          m_indexCount(indexCount) {}

    [[nodiscard]] static tl::expected<Model, Error<EngineError>> loadFromFile(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, std::string const& filepath);

    [[nodiscard]] static tl::expected<Model, Error<EngineError>> create(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, std::vector<Vertex> const& vertices,
        std::vector<u32> const& indices);

    [[nodiscard]] Buffer const& vertexBuffer() const noexcept {
        return m_vertexBuffer;
    }
    [[nodiscard]] Buffer const& indexBuffer() const noexcept {
        return m_indexBuffer;
    }
    [[nodiscard]] u32 indexCount() const noexcept { return m_indexCount; }

   private:
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    u32 m_indexCount{0};
};

}  // namespace mpvgl::vlk
