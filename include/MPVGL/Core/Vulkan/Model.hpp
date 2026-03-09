#pragma once

#include <string>

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"

namespace mpvgl::vlk {

class Model {
   public:
    Model() = default;
    Model(Buffer vertexBuffer, Buffer indexBuffer, uint32_t indexCount)
        : m_vertexBuffer(std::move(vertexBuffer)),
          m_indexBuffer(std::move(indexBuffer)),
          m_indexCount(indexCount) {}

    [[nodiscard]] static tl::expected<Model, Error> loadFromFile(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, std::string const& filepath);

    [[nodiscard]] Buffer const& vertexBuffer() const noexcept {
        return m_vertexBuffer;
    }
    [[nodiscard]] Buffer const& indexBuffer() const noexcept {
        return m_indexBuffer;
    }
    [[nodiscard]] uint32_t indexCount() const noexcept { return m_indexCount; }

   private:
    Buffer m_vertexBuffer{};
    Buffer m_indexBuffer{};
    uint32_t m_indexCount{0};
};

}  // namespace mpvgl::vlk
