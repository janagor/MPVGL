#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class Texture {
   public:
    Texture() = default;
    Texture(Texture const&) = delete;
    Texture& operator=(Texture const&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    ~Texture();

    [[nodiscard]] static tl::expected<Texture, Error<EngineError>> loadFromFile(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, std::string const& filepath);
    [[nodiscard]] static tl::expected<Texture, Error<EngineError>>
    createDepthTexture(DeviceContext const& device, uint32_t width,
                       uint32_t height, VkFormat format);

    [[nodiscard]] VkImage handle() const noexcept { return m_image; }
    [[nodiscard]] VkImageView imageView() const noexcept { return m_imageView; }
    [[nodiscard]] VkSampler sampler() const noexcept { return m_sampler; }
    [[nodiscard]] uint32_t mipLevels() const noexcept { return m_mipLevels; }

   private:
    VmaAllocator m_allocator{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp{};

    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_imageView{VK_NULL_HANDLE};
    VkSampler m_sampler{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};
    uint32_t m_mipLevels{0};

   private:
    Texture(VkImage image, VkImageView imageView, VkSampler sampler,
            VmaAllocation allocation, uint32_t mipLevels,
            VmaAllocator allocator, vkb::DispatchTable disp) noexcept;

    void cleanup() noexcept;

    static tl::expected<void, Error<EngineError>> transitionImageLayout(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

    static tl::expected<void, Error<EngineError>> generateMipmaps(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, VkImage image, VkFormat imageFormat,
        int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    static void copyBufferToImage(DeviceContext const& device,
                                  VkCommandPool commandPool,
                                  VkQueue graphicsQueue, VkBuffer buffer,
                                  VkImage image, uint32_t width,
                                  uint32_t height);

   private:
    struct RawPixels {
        unsigned char* data{nullptr};
        int width{0};
        int height{0};
        uint32_t mipLevels{0};

        [[nodiscard]] VkDeviceSize size() const noexcept {
            return width * height * 4;
        }
        void free() noexcept;
    };

    static tl::expected<RawPixels, Error<EngineError>> loadRawPixels(
        std::string const& filepath);

    static tl::expected<std::pair<VkImage, VmaAllocation>, Error<EngineError>>
    createAllocatedImage(DeviceContext const& device, uint32_t width,
                         uint32_t height, uint32_t mipLevels);

    static tl::expected<void, Error<EngineError>> uploadAndGenerateMipmaps(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, VkImage image, RawPixels const& pixels);

    static tl::expected<VkImageView, Error<EngineError>> createImageView(
        DeviceContext const& device, VkImage image, uint32_t mipLevels);

    static tl::expected<VkSampler, Error<EngineError>> createSampler(
        DeviceContext const& device);
};

}  // namespace mpvgl::vlk
