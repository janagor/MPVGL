#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Renderer.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

auto createShaderModule(DeviceContext const &context,
                        std::span<std::byte const> code) -> VkShaderModule;
auto createBuffer(Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage,
                  VmaMemoryUsage memoryUsage,
                  VmaAllocationCreateFlags allocFlags, VkBuffer &buffer,
                  VmaAllocation &bufferAllocation)
    -> tl::expected<void, Error<EngineError>>;
auto beginSingleTimeCommands(Vulkan &vulkan) -> VkCommandBuffer;
void endSingleTimeCommands(Vulkan &vulkan, VkCommandBuffer commandBuffer);
void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                 VkDeviceSize size);
auto createImage(Vulkan &vulkan, Extent2D const &extent, u32 mipLevels,
                 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                 VmaMemoryUsage memoryUsage,
                 VmaAllocationCreateFlags allocFlags, VkImage &image,
                 VmaAllocation &imageAllocation)
    -> tl::expected<void, Error<EngineError>>;
auto createImageView(Vulkan &vulkan, VkImage image, VkFormat format,
                     VkImageAspectFlags aspectFlags, u32 mipLevels)
    -> tl::expected<VkImageView, Error<EngineError>>;
auto createImageViews(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>>;
auto findSupportedFormat(Vulkan &vulkan,
                         std::vector<VkFormat> const &candidates,
                         VkImageTiling tiling, VkFormatFeatureFlags features)
    -> tl::expected<VkFormat, Error<EngineError>>;
auto findDepthFormat(Vulkan &vulkan)
    -> tl::expected<VkFormat, Error<EngineError>>;
auto has_stencil_component(VkFormat format) -> bool;
void cleanupSwapChain(Vulkan &vulkan);
auto recreateSwapchain(Renderer &renderer)
    -> tl::expected<void, Error<EngineError>>;
auto recordCommandBuffer(Vulkan &vulkan, VkCommandBuffer command_buffer,
                         u32 image_index)
    -> tl::expected<void, Error<EngineError>>;
void updateUniformBuffer(Vulkan &vulkan, u32 current_image);

}  // namespace mpvgl::vlk
