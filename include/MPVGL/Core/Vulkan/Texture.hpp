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
    class RawPixels {
       public:
        constexpr RawPixels() noexcept = default;

        constexpr RawPixels(unsigned char* data, int width, int height,
                            u32 mipLevels) noexcept
            : m_data(data),
              m_width(width),
              m_height(height),
              m_mipLevels(mipLevels) {}

        [[nodiscard]] unsigned char* data() const noexcept { return m_data; }
        [[nodiscard]] int width() const noexcept { return m_width; }
        [[nodiscard]] int height() const noexcept { return m_height; }
        [[nodiscard]] u32 mipLevels() const noexcept { return m_mipLevels; }

        [[nodiscard]] unsigned char*& data() noexcept { return m_data; }
        [[nodiscard]] int& width() noexcept { return m_width; }
        [[nodiscard]] int& height() noexcept { return m_height; }
        [[nodiscard]] u32& mipLevels() noexcept { return m_mipLevels; }

        [[nodiscard]] VkDeviceSize size() const noexcept {
            return static_cast<VkDeviceSize>(m_width) * m_height * 4;
        }

        void free() noexcept;

       private:
        unsigned char* m_data{nullptr};
        int m_width{0};
        int m_height{0};
        u32 m_mipLevels{0};
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
