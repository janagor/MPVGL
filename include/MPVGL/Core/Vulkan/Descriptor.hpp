#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <vector>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class DescriptorSetLayout {
   public:
    DescriptorSetLayout() = default;
    ~DescriptorSetLayout() noexcept;

    DescriptorSetLayout(DescriptorSetLayout const&) = delete;
    DescriptorSetLayout& operator=(DescriptorSetLayout const&) = delete;

    DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
    DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

    [[nodiscard]] VkDescriptorSetLayout handle() const noexcept {
        return m_layout;
    }

    DescriptorSetLayout(VkDescriptorSetLayout layout,
                        vkb::DispatchTable disp) noexcept;

   private:
    void cleanup() noexcept;

    VkDescriptorSetLayout m_layout{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp{};
};

class DescriptorLayoutBuilder {
   public:
    DescriptorLayoutBuilder& addBinding(std::uint32_t binding,
                                        VkDescriptorType type,
                                        VkShaderStageFlags stageFlags);
    void clear();
    tl::expected<DescriptorSetLayout, Error<EngineError>> build(
        DeviceContext const& device);

   private:
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

class DescriptorAllocator {
   public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    tl::expected<void, Error<EngineError>> init(
        DeviceContext const& device, uint32_t maxSets,
        std::span<PoolSizeRatio> poolRatios);
    void cleanup(DeviceContext const& device);

    tl::expected<VkDescriptorSet, Error<EngineError>> allocate(
        DeviceContext const& device, VkDescriptorSetLayout layout);

   private:
    VkDescriptorPool m_pool{VK_NULL_HANDLE};
};

class DescriptorWriter {
   public:
    DescriptorWriter& writeBuffer(uint32_t binding, VkBuffer buffer,
                                  size_t size, size_t offset,
                                  VkDescriptorType type);
    DescriptorWriter& writeImage(uint32_t binding, VkImageView imageView,
                                 VkSampler sampler, VkImageLayout layout,
                                 VkDescriptorType type);

    void updateSet(DeviceContext const& device, VkDescriptorSet set);
    void clear();

   private:
    std::vector<VkWriteDescriptorSet> m_writes;
    std::deque<VkDescriptorImageInfo> m_imageInfos;
    std::deque<VkDescriptorBufferInfo> m_bufferInfos;
};

}  // namespace mpvgl::vlk
