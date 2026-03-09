#pragma once

#include <vector>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"

namespace mpvgl::vlk {

class Buffer {
   public:
    Buffer() = default;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;
    ~Buffer();

    static tl::expected<Buffer, Error> create(
        DeviceContext const& device, VkDeviceSize size,
        VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
        VmaAllocationCreateFlags allocFlags = 0);

    template <typename T>
    static tl::expected<Buffer, Error> createFromData(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, std::vector<T> const& data,
        VkBufferUsageFlags usage) {
        VkDeviceSize bufferSize = sizeof(T) * data.size();
        return createWithStaging(device, commandPool, graphicsQueue,
                                 data.data(), bufferSize, usage);
    }

    [[nodiscard]] VkBuffer handle() const noexcept;
    [[nodiscard]] VmaAllocation allocation() const noexcept;
    [[nodiscard]] VkDeviceSize size() const noexcept;

    [[nodiscard]] tl::expected<void*, Error> map() noexcept;
    void unmap() noexcept;

   private:
    VmaAllocator m_allocator{VK_NULL_HANDLE};

    VkBuffer m_buffer{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};
    VkDeviceSize m_size{0};

   private:
    Buffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size,
           VmaAllocator allocator) noexcept;
    static tl::expected<Buffer, Error> createWithStaging(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, const void* data, VkDeviceSize size,
        VkBufferUsageFlags usage);
    static void copyBuffer(DeviceContext const& device,
                           VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkBuffer srcBuffer, VkBuffer dstBuffer,
                           VkDeviceSize size);
    void cleanup() noexcept;
};

}  // namespace mpvgl::vlk
