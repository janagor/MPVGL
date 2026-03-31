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

    [[nodiscard]] static auto loadFromFile(DeviceContext const& device,
                                           VkCommandPool commandPool,
                                           VkQueue graphicsQueue,
                                           std::string const& filepath)
        -> tl::expected<Model, Error<EngineError>>;

    [[nodiscard]] static auto create(DeviceContext const& device,
                                     VkCommandPool commandPool,
                                     VkQueue graphicsQueue,
                                     std::vector<Vertex> const& vertices,
                                     std::vector<u32> const& indices)
        -> tl::expected<Model, Error<EngineError>>;

    [[nodiscard]] auto vertexBuffer() const noexcept -> Buffer const& {
        return m_vertexBuffer;
    }
    [[nodiscard]] auto indexBuffer() const noexcept -> Buffer const& {
        return m_indexBuffer;
    }
    [[nodiscard]] auto indexCount() const noexcept -> u32 {
        return m_indexCount;
    }

   private:
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    u32 m_indexCount{0};
};

}  // namespace mpvgl::vlk
