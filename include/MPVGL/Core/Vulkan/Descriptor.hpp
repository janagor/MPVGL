#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"

namespace mpvgl::vlk {

class DescriptorLayoutBuilder {
   public:
    DescriptorLayoutBuilder& addBinding(std::uint32_t binding,
                                        VkDescriptorType type,
                                        VkShaderStageFlags stageFlags);
    void clear();
    tl::expected<VkDescriptorSetLayout, Error> build(
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

    tl::expected<void, Error> init(DeviceContext const& device,
                                   uint32_t maxSets,
                                   std::span<PoolSizeRatio> poolRatios);
    void cleanup(DeviceContext const& device);

    tl::expected<VkDescriptorSet, Error> allocate(DeviceContext const& device,
                                                  VkDescriptorSetLayout layout);

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
