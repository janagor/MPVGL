#pragma once

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
#include "MPVGL/Utility/Types.hpp"

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
    createDepthTexture(DeviceContext const& device, u32 width, u32 height,
                       VkFormat format);

    [[nodiscard]] VkImage handle() const noexcept { return m_image; }
    [[nodiscard]] VkImageView imageView() const noexcept { return m_imageView; }
    [[nodiscard]] VkSampler sampler() const noexcept { return m_sampler; }
    [[nodiscard]] u32 mipLevels() const noexcept { return m_mipLevels; }

   private:
    VmaAllocator m_allocator{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp{};

    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_imageView{VK_NULL_HANDLE};
    VkSampler m_sampler{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};
    u32 m_mipLevels{0};

   private:
    Texture(VkImage image, VkImageView imageView, VkSampler sampler,
            VmaAllocation allocation, u32 mipLevels, VmaAllocator allocator,
            vkb::DispatchTable disp) noexcept;

    void cleanup() noexcept;

    static tl::expected<void, Error<EngineError>> transitionImageLayout(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels);

    static tl::expected<void, Error<EngineError>> generateMipmaps(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, VkImage image, VkFormat imageFormat,
        i32 texWidth, i32 texHeight, u32 mipLevels);

    static void copyBufferToImage(DeviceContext const& device,
                                  VkCommandPool commandPool,
                                  VkQueue graphicsQueue, VkBuffer buffer,
                                  VkImage image, u32 width, u32 height);

   private:
    struct RawPixels {
        unsigned char* data{nullptr};
        int width{0};
        int height{0};
        u32 mipLevels{0};

        [[nodiscard]] VkDeviceSize size() const noexcept {
            return width * height * 4;
        }
        void free() noexcept;
    };

    static tl::expected<RawPixels, Error<EngineError>> loadRawPixels(
        std::string const& filepath);

    static tl::expected<std::pair<VkImage, VmaAllocation>, Error<EngineError>>
    createAllocatedImage(DeviceContext const& device, u32 width, u32 height,
                         u32 mipLevels);

    static tl::expected<void, Error<EngineError>> uploadAndGenerateMipmaps(
        DeviceContext const& device, VkCommandPool commandPool,
        VkQueue graphicsQueue, VkImage image, RawPixels const& pixels);

    static tl::expected<VkImageView, Error<EngineError>> createImageView(
        DeviceContext const& device, VkImage image, u32 mipLevels);

    static tl::expected<VkSampler, Error<EngineError>> createSampler(
        DeviceContext const& device);
};

}  // namespace mpvgl::vlk
