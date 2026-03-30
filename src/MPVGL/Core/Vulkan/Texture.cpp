#include <cmath>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <utility>

#include <stb/stb_image.h>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Graphics/Extent.hpp"
#include "MPVGL/IO/ResourceBuffer.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

namespace {

template <typename F>
void executeSingleTimeCommands(DeviceContext const& device,
                               VkCommandPool commandPool, VkQueue graphicsQueue,
                               F&& action) {
    auto allocInfo = initializers::commandBufferAllocateInfo(
        commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkCommandBuffer commandBuffer;
    device.logDevDisp.allocateCommandBuffers(&allocInfo, &commandBuffer);

    auto beginInfo = initializers::commandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    device.logDevDisp.beginCommandBuffer(commandBuffer, &beginInfo);

    action(commandBuffer);

    device.logDevDisp.endCommandBuffer(commandBuffer);
    auto submitInfo = initializers::submitInfo({}, {}, {&commandBuffer, 1}, {});
    device.logDevDisp.queueSubmit(graphicsQueue, 1, &submitInfo,
                                  VK_NULL_HANDLE);
    device.logDevDisp.queueWaitIdle(graphicsQueue);
    device.logDevDisp.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

}  // namespace

Texture::Texture(VkImage image, VkImageView imageView, VkSampler sampler,
                 VmaAllocation allocation, u32 mipLevels,
                 VmaAllocator allocator, vkb::DispatchTable disp) noexcept
    : m_image(image),
      m_imageView(imageView),
      m_sampler(sampler),
      m_allocation(allocation),
      m_mipLevels(mipLevels),
      m_allocator(allocator),
      m_disp(disp) {}

Texture::Texture(Texture&& other) noexcept
    : m_image(std::exchange(other.m_image, VK_NULL_HANDLE)),
      m_imageView(std::exchange(other.m_imageView, VK_NULL_HANDLE)),
      m_sampler(std::exchange(other.m_sampler, VK_NULL_HANDLE)),
      m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE)),
      m_mipLevels(other.m_mipLevels),
      m_allocator(other.m_allocator),
      m_disp(other.m_disp) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
        m_imageView = std::exchange(other.m_imageView, VK_NULL_HANDLE);
        m_sampler = std::exchange(other.m_sampler, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
        m_mipLevels = other.m_mipLevels;
        m_allocator = other.m_allocator;
        m_disp = other.m_disp;
    }
    return *this;
}

Texture::~Texture() { cleanup(); }

void Texture::cleanup() noexcept {
    if (m_sampler != VK_NULL_HANDLE) m_disp.destroySampler(m_sampler, nullptr);
    if (m_imageView != VK_NULL_HANDLE)
        m_disp.destroyImageView(m_imageView, nullptr);
    if (m_image != VK_NULL_HANDLE && m_allocator != VK_NULL_HANDLE)
        vmaDestroyImage(m_allocator, m_image, m_allocation);
    m_sampler = VK_NULL_HANDLE;
    m_imageView = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
}

tl::expected<void, Error<EngineError>> Texture::transitionImageLayout(
    DeviceContext const& device, VkCommandPool commandPool,
    VkQueue graphicsQueue, VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels) {
    tl::expected<void, Error<EngineError>> result = {};

    executeSingleTimeCommands(
        device, commandPool, graphicsQueue, [&](VkCommandBuffer cmd) {
            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            if (format == VK_FORMAT_D32_SFLOAT ||
                format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                format == VK_FORMAT_D24_UNORM_S8_UINT) {
                aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                    format == VK_FORMAT_D24_UNORM_S8_UINT) {
                    aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }

            auto subresourceRange = VkImageSubresourceRange{
                .aspectMask = aspectMask,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            auto barrier = initializers::imageMemoryBarrier(
                oldLayout, newLayout, image, subresourceRange);
            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                       newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else {
                result = tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                              "Unsupported layout transition"}};
                return;
            }

            device.logDevDisp.cmdPipelineBarrier(
                cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr,
                1, &barrier);
        });
    return result;
}

void Texture::copyBufferToImage(DeviceContext const& device,
                                VkCommandPool commandPool,
                                VkQueue graphicsQueue, VkBuffer buffer,
                                VkImage image, Extent2D const& extent) {
    executeSingleTimeCommands(
        device, commandPool, graphicsQueue, [&](VkCommandBuffer cmd) {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {extent.width, extent.height, 1};

            device.logDevDisp.cmdCopyBufferToImage(
                cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                &region);
        });
}

tl::expected<void, Error<EngineError>> Texture::generateMipmaps(
    DeviceContext const& device, VkCommandPool commandPool,
    VkQueue graphicsQueue, VkImage image, VkFormat imageFormat,
    Extent2D const& extent, u32 mipLevels) {
    VkFormatProperties formatProperties;
    device.instDisp.getPhysicalDeviceFormatProperties(
        device.logicalDevice.physical_device, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures &
          VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        return tl::unexpected{
            Error{EngineError::VulkanRuntimeError,
                  "Texture image format does not support linear blitting"}};
    }

    executeSingleTimeCommands(
        device, commandPool, graphicsQueue, [&](VkCommandBuffer cmd) {
            auto subresourceRange = VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            auto barrier = initializers::imageMemoryBarrier(
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, subresourceRange);

            auto mipWidth = static_cast<i32>(extent.width);
            auto mipHeight = static_cast<i32>(extent.height);

            for (u32 i = 1; i < mipLevels; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                device.logDevDisp.cmdPipelineBarrier(
                    cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                    1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                                      mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                device.logDevDisp.cmdBlitImage(
                    cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                    VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                device.logDevDisp.cmdPipelineBarrier(
                    cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                    nullptr, 1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            device.logDevDisp.cmdPipelineBarrier(
                cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);
        });

    return {};
}

tl::expected<Texture, Error<EngineError>> Texture::loadFromFile(
    DeviceContext const& device, VkCommandPool commandPool,
    VkQueue graphicsQueue, std::string const& filepath) {
    return io::ResourceBuffer::load(filepath)
        .map_error([](auto const& e) {
            return Error<EngineError>{EngineError::FileNotFound, e.message()};
        })
        .and_then([&](io::ResourceBuffer const& buffer)
                      -> tl::expected<Texture, Error<EngineError>> {
            auto view = buffer.view();
            int width, height, channels;

            unsigned char* data = stbi_load_from_memory(
                reinterpret_cast<stbi_uc const*>(view.data()),
                static_cast<int>(view.size()), &width, &height, &channels,
                STBI_rgb_alpha);
            auto extent = Extent2D{
                .width = static_cast<u32>(width),
                .height = static_cast<u32>(height),
            };

            if (!data) {
                return tl::unexpected{
                    Error{EngineError::VulkanRuntimeError,
                          "Failed to decode texture: " + filepath}};
            }

            u32 const mipLevels = static_cast<u32>(std::floor(
                                      std::log2(std::max(width, height)))) +
                                  1;
            RawPixels pixels{data, extent, mipLevels};

            auto imageRes = createAllocatedImage(device, extent, mipLevels);
            if (!imageRes) {
                pixels.free();
                return tl::unexpected{imageRes.error()};
            }
            auto [image, allocation] = imageRes.value();

            auto uploadRes = uploadAndGenerateMipmaps(
                device, commandPool, graphicsQueue, image, pixels);

            pixels.free();

            if (!uploadRes) {
                vmaDestroyImage(device.allocator, image, allocation);
                return tl::unexpected{uploadRes.error()};
            }

            auto viewRes = createImageView(device, image, mipLevels);
            if (!viewRes) {
                vmaDestroyImage(device.allocator, image, allocation);
                return tl::unexpected{viewRes.error()};
            }

            auto samplerRes = createSampler(device);
            if (!samplerRes) {
                device.logDevDisp.destroyImageView(viewRes.value(), nullptr);
                vmaDestroyImage(device.allocator, image, allocation);
                return tl::unexpected{samplerRes.error()};
            }

            return Texture(image, viewRes.value(), samplerRes.value(),
                           allocation, mipLevels, device.allocator,
                           device.logDevDisp);
        });
}

void Texture::RawPixels::free() noexcept {
    if (m_data) {
        stbi_image_free(m_data);
        m_data = nullptr;
    }
}

tl::expected<Texture::RawPixels, Error<EngineError>> Texture::loadRawPixels(
    std::string const& filepath) {
    RawPixels pixels{};
    int channels;
    auto const& extent = pixels.extent();
    auto width = static_cast<int>(extent.width),
         height = static_cast<int>(extent.height);
    pixels.data() =
        stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels.data()) {
        return tl::unexpected{
            Error{EngineError::VulkanRuntimeError,
                  "Failed to load Texture from: " + filepath}};
    }
    pixels.mipLevels() = static_cast<u32>(std::floor(std::log2(std::max(
                             pixels.extent().width, pixels.extent().height)))) +
                         1;
    return pixels;
}

tl::expected<std::pair<VkImage, VmaAllocation>, Error<EngineError>>
Texture::createAllocatedImage(DeviceContext const& device,
                              Extent2D const& extent, u32 mipLevels) {
    auto imageInfo = initializers::imageCreateInfo();
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {extent.width, extent.height, 1};
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImage image;
    VmaAllocation allocation;
    if (vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &image,
                       &allocation, nullptr) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Image via VMA"}};
    }
    return std::make_pair(image, allocation);
}

tl::expected<void, Error<EngineError>> Texture::uploadAndGenerateMipmaps(
    DeviceContext const& device, VkCommandPool commandPool,
    VkQueue graphicsQueue, VkImage image, RawPixels const& pixels) {
    auto stagingRes =
        Buffer::create(device, pixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VMA_MEMORY_USAGE_AUTO,
                       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    if (!stagingRes) return tl::unexpected{stagingRes.error()};
    auto stagingBuffer = std::move(stagingRes.value());

    if (auto mapRes = stagingBuffer.map()) {
        std::memcpy(mapRes.value(), pixels.data(),
                    static_cast<size_t>(pixels.size()));
        stagingBuffer.unmap();
    } else {
        return tl::unexpected{mapRes.error()};
    }

    if (auto res = transitionImageLayout(
            device, commandPool, graphicsQueue, image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            pixels.mipLevels());
        !res) {
        return res;
    }

    copyBufferToImage(device, commandPool, graphicsQueue,
                      stagingBuffer.handle(), image, pixels.extent());

    return generateMipmaps(device, commandPool, graphicsQueue, image,
                           VK_FORMAT_R8G8B8A8_SRGB, pixels.extent(),
                           pixels.mipLevels());
}

tl::expected<VkImageView, Error<EngineError>> Texture::createImageView(
    DeviceContext const& device, VkImage image, u32 mipLevels) {
    auto subresourceRange = VkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    auto viewInfo = initializers::imageViewCreateInfo(
        image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
        subresourceRange);

    VkImageView imageView;
    if (device.logDevDisp.createImageView(&viewInfo, nullptr, &imageView) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Texture Image View"}};
    }
    return imageView;
}

tl::expected<VkSampler, Error<EngineError>> Texture::createSampler(
    DeviceContext const& device) {
    VkPhysicalDeviceProperties properties{};
    device.instDisp.getPhysicalDeviceProperties(
        device.logicalDevice.physical_device, &properties);

    auto samplerInfo = initializers::samplerCreateInfo();
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    if (device.logDevDisp.createSampler(&samplerInfo, nullptr, &sampler) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Texture Sampler"}};
    }
    return sampler;
}

tl::expected<Texture, Error<EngineError>> Texture::createDepthTexture(
    DeviceContext const& device, u32 width, u32 height, VkFormat format) {
    auto imageInfo = initializers::imageCreateInfo();
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImage image;
    VmaAllocation allocation;
    if (vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &image,
                       &allocation, nullptr) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Depth Image via VMA"}};
    }

    auto subresourceRange = VkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    auto viewInfo = initializers::imageViewCreateInfo(
        image, VK_IMAGE_VIEW_TYPE_2D, format, subresourceRange);

    VkImageView imageView;
    if (device.logDevDisp.createImageView(&viewInfo, nullptr, &imageView) !=
        VK_SUCCESS) {
        vmaDestroyImage(device.allocator, image, allocation);
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Depth Image View"}};
    }

    return Texture(image, imageView, VK_NULL_HANDLE, allocation, 1,
                   device.allocator, device.logDevDisp);
}

}  // namespace mpvgl::vlk
