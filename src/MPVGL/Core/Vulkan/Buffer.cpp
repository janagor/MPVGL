#include <cstddef>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <utility>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

Buffer::Buffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size,
               VmaAllocator allocator) noexcept
    : m_buffer{buffer},
      m_allocation{allocation},
      m_size{size},
      m_allocator{allocator} {}

Buffer::Buffer(Buffer&& other) noexcept
    : m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE)),
      m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE)),
      m_size(other.m_size),
      m_allocator(other.m_allocator) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
        m_size = other.m_size;
        m_allocator = other.m_allocator;
    }
    return *this;
}

Buffer::~Buffer() { cleanup(); }

tl::expected<Buffer, Error<EngineError>> Buffer::create(
    DeviceContext const& device, VkDeviceSize size, VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags) {
    auto bufferInfo = initializers::bufferCreateInfo(size, usage);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = allocFlags;

    VkBuffer buffer;
    VmaAllocation allocation;

    if (vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &buffer,
                        &allocation, nullptr) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Buffer via VMA"}};
    }
    return Buffer{buffer, allocation, size, device.allocator};
}

VkBuffer Buffer::handle() const noexcept { return m_buffer; };

VmaAllocation Buffer::allocation() const noexcept { return m_allocation; }

VkDeviceSize Buffer::size() const noexcept { return m_size; }

tl::expected<void*, Error<EngineError>> Buffer::map() noexcept {
    if (m_allocation == VK_NULL_HANDLE || m_allocator == VK_NULL_HANDLE) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Cannot map an uninitialized buffer"}};
    }

    void* mappedData = nullptr;  // NOLINT(misc-const-correctness)
    if (vmaMapMemory(m_allocator, m_allocation, &mappedData) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to map Buffer memory via VMA"}};
    }

    return mappedData;
}

void Buffer::unmap() noexcept {
    if (m_allocation != VK_NULL_HANDLE && m_allocator != VK_NULL_HANDLE) {
        vmaUnmapMemory(m_allocator, m_allocation);
    }
}

void Buffer::copyBuffer(DeviceContext const& device, VkCommandPool commandPool,
                        VkQueue graphicsQueue, VkBuffer srcBuffer,
                        VkBuffer dstBuffer, VkDeviceSize size) {
    auto allocInfo = initializers::commandBufferAllocateInfo(
        commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkCommandBuffer commandBuffer;
    device.logDevDisp.allocateCommandBuffers(&allocInfo, &commandBuffer);

    auto beginInfo = initializers::commandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    device.logDevDisp.beginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    device.logDevDisp.cmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1,
                                    &copyRegion);

    device.logDevDisp.endCommandBuffer(commandBuffer);

    auto submitInfo = initializers::submitInfo({}, {}, {&commandBuffer, 1}, {});
    device.logDevDisp.queueSubmit(graphicsQueue, 1, &submitInfo,
                                  VK_NULL_HANDLE);
    device.logDevDisp.queueWaitIdle(graphicsQueue);

    device.logDevDisp.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

tl::expected<Buffer, Error<EngineError>> Buffer::createWithStaging(
    DeviceContext const& device, VkCommandPool commandPool,
    VkQueue graphicsQueue, void const* data, VkDeviceSize size,
    VkBufferUsageFlags usage) {
    return Buffer::create(
               device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VMA_MEMORY_USAGE_AUTO,
               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
        .and_then([&](Buffer staging) {
            return staging.map().transform([&](void* d) {
                std::memcpy(d, data, static_cast<size_t>(size));
                staging.unmap();
                return std::move(staging);
            });
        })
        .and_then([&](Buffer staging) {
            return Buffer::create(device, size,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                                  VMA_MEMORY_USAGE_AUTO, 0)
                .transform([&](Buffer gpu) {
                    Buffer::copyBuffer(device, commandPool, graphicsQueue,
                                       staging.handle(), gpu.handle(), size);
                    return std::move(gpu);
                });
        });
}

void Buffer::cleanup() noexcept {
    if (m_buffer != VK_NULL_HANDLE && m_allocator != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}

}  // namespace mpvgl::vlk
