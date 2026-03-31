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
#include "MPVGL/Graphics/Extent.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

class Texture {
   public:
    Texture() = default;
    Texture(Texture const&) = delete;
    auto operator=(Texture const&) -> Texture& = delete;
    Texture(Texture&& other) noexcept;
    auto operator=(Texture&& other) noexcept -> Texture&;
    ~Texture();

    [[nodiscard]] static auto loadFromFile(DeviceContext const& device,
                                           VkCommandPool commandPool,
                                           VkQueue graphicsQueue,
                                           std::string const& filepath)
        -> tl::expected<Texture, Error<EngineError>>;
    [[nodiscard]] static auto createDepthTexture(DeviceContext const& device,
                                                 u32 width, u32 height,
                                                 VkFormat format)
        -> tl::expected<Texture, Error<EngineError>>;

    [[nodiscard]] auto handle() const noexcept -> VkImage { return m_image; }
    [[nodiscard]] auto imageView() const noexcept -> VkImageView {
        return m_imageView;
    }
    [[nodiscard]] auto sampler() const noexcept -> VkSampler {
        return m_sampler;
    }
    [[nodiscard]] auto mipLevels() const noexcept -> u32 { return m_mipLevels; }

   private:
    VmaAllocator m_allocator{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;

    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_imageView{VK_NULL_HANDLE};
    VkSampler m_sampler{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};
    u32 m_mipLevels{0};

    Texture(VkImage image, VkImageView imageView, VkSampler sampler,
            VmaAllocation allocation, u32 mipLevels, VmaAllocator allocator,
            vkb::DispatchTable disp) noexcept;

    void cleanup() noexcept;

    static auto transitionImageLayout(DeviceContext const& device,
                                      VkCommandPool commandPool,
                                      VkQueue graphicsQueue, VkImage image,
                                      VkFormat format, VkImageLayout oldLayout,
                                      VkImageLayout newLayout, u32 mipLevels)
        -> tl::expected<void, Error<EngineError>>;

    static auto generateMipmaps(DeviceContext const& device,
                                VkCommandPool commandPool,
                                VkQueue graphicsQueue, VkImage image,
                                VkFormat imageFormat, Extent2D const& extent,
                                u32 mipLevels)
        -> tl::expected<void, Error<EngineError>>;

    static void copyBufferToImage(DeviceContext const& device,
                                  VkCommandPool commandPool,
                                  VkQueue graphicsQueue, VkBuffer buffer,
                                  VkImage image, Extent2D const& extent);

    class RawPixels {
       public:
        constexpr RawPixels() noexcept = default;

        constexpr RawPixels(unsigned char* data, Extent2D const& extent,
                            u32 mipLevels) noexcept
            : m_data(data), m_extent(extent), m_mipLevels(mipLevels) {}

        [[nodiscard]] auto data() const noexcept -> unsigned char* {
            return m_data;
        }
        [[nodiscard]] auto extent() const noexcept -> Extent2D const& {
            return m_extent;
        }
        [[nodiscard]] auto mipLevels() const noexcept -> u32 {
            return m_mipLevels;
        }

        [[nodiscard]] auto data() noexcept -> unsigned char*& { return m_data; }
        [[nodiscard]] auto extent() noexcept -> Extent2D& { return m_extent; }
        [[nodiscard]] auto mipLevels() noexcept -> u32& { return m_mipLevels; }

        [[nodiscard]] auto size() const noexcept -> VkDeviceSize {
            return static_cast<VkDeviceSize>(m_extent.width) * m_extent.height *
                   4;
        }

        void free() noexcept;

       private:
        unsigned char* m_data{nullptr};
        Extent2D m_extent{};
        u32 m_mipLevels{0};
    };

    static auto loadRawPixels(std::string const& filepath)
        -> tl::expected<RawPixels, Error<EngineError>>;

    static auto createAllocatedImage(DeviceContext const& device,
                                     Extent2D const& extent, u32 mipLevels)
        -> tl::expected<std::pair<VkImage, VmaAllocation>, Error<EngineError>>;

    static auto uploadAndGenerateMipmaps(DeviceContext const& device,
                                         VkCommandPool commandPool,
                                         VkQueue graphicsQueue, VkImage image,
                                         RawPixels const& pixels)
        -> tl::expected<void, Error<EngineError>>;

    static auto createImageView(DeviceContext const& device, VkImage image,
                                u32 mipLevels)
        -> tl::expected<VkImageView, Error<EngineError>>;

    static auto createSampler(DeviceContext const& device)
        -> tl::expected<VkSampler, Error<EngineError>>;
};

}  // namespace mpvgl::vlk
