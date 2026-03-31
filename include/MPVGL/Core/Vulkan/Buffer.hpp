#pragma once

#include <vector>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class Buffer {
   public:
    Buffer() = default;
    Buffer(Buffer const&) = delete;
    auto operator=(Buffer const&) -> Buffer& = delete;
    Buffer(Buffer&& other) noexcept;
    auto operator=(Buffer&& other) noexcept -> Buffer&;
    ~Buffer();

    static auto create(DeviceContext const& device, VkDeviceSize size,
                       VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                       VmaAllocationCreateFlags allocFlags = 0)
        -> tl::expected<Buffer, Error<EngineError>>;

    template <typename T>
    static auto createFromData(DeviceContext const& device,
                               VkCommandPool commandPool, VkQueue graphicsQueue,
                               std::vector<T> const& data,
                               VkBufferUsageFlags usage)
        -> tl::expected<Buffer, Error<EngineError>> {
        VkDeviceSize const bufferSize = sizeof(T) * data.size();
        return createWithStaging(device, commandPool, graphicsQueue,
                                 data.data(), bufferSize, usage);
    }

    [[nodiscard]] auto handle() const noexcept -> VkBuffer;
    [[nodiscard]] auto allocation() const noexcept -> VmaAllocation;
    [[nodiscard]] auto size() const noexcept -> VkDeviceSize;

    [[nodiscard]] auto map() noexcept
        -> tl::expected<void*, Error<EngineError>>;
    void unmap() noexcept;

   private:
    VmaAllocator m_allocator{VK_NULL_HANDLE};

    VkBuffer m_buffer{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};
    VkDeviceSize m_size{0};

    Buffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size,
           VmaAllocator allocator) noexcept;
    static auto createWithStaging(DeviceContext const& device,
                                  VkCommandPool commandPool,
                                  VkQueue graphicsQueue, void const* data,
                                  VkDeviceSize size, VkBufferUsageFlags usage)
        -> tl::expected<Buffer, Error<EngineError>>;
    static void copyBuffer(DeviceContext const& device,
                           VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkBuffer srcBuffer, VkBuffer dstBuffer,
                           VkDeviceSize size);
    void cleanup() noexcept;
};

}  // namespace mpvgl::vlk
