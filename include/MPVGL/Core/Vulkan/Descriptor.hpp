#pragma once

#include <cstddef>
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
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

class DescriptorSetLayout {
   public:
    DescriptorSetLayout() = default;
    ~DescriptorSetLayout() noexcept;

    DescriptorSetLayout(DescriptorSetLayout const&) = delete;
    auto operator=(DescriptorSetLayout const&) -> DescriptorSetLayout& = delete;

    DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
    auto operator=(DescriptorSetLayout&& other) noexcept
        -> DescriptorSetLayout&;

    [[nodiscard]] auto handle() const noexcept -> VkDescriptorSetLayout {
        return m_layout;
    }

    DescriptorSetLayout(VkDescriptorSetLayout layout,
                        vkb::DispatchTable disp) noexcept;

   private:
    void cleanup() noexcept;

    VkDescriptorSetLayout m_layout{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;
};

class DescriptorLayoutBuilder {
   public:
    auto addBinding(u32 binding, VkDescriptorType type,
                    VkShaderStageFlags stageFlags) -> DescriptorLayoutBuilder&;
    void clear();
    auto build(DeviceContext const& device)
        -> tl::expected<DescriptorSetLayout, Error<EngineError>>;

   private:
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

class DescriptorAllocator {
   public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        f64 ratio;
    };

    auto init(DeviceContext const& device, u32 maxSets,
              std::span<PoolSizeRatio> poolRatios)
        -> tl::expected<void, Error<EngineError>>;
    void cleanup(DeviceContext const& device);

    auto allocate(DeviceContext const& device, VkDescriptorSetLayout layout)
        -> tl::expected<VkDescriptorSet, Error<EngineError>>;

   private:
    VkDescriptorPool m_pool{VK_NULL_HANDLE};
};

class DescriptorWriter {
   public:
    auto writeBuffer(u32 binding, VkBuffer buffer, size_t size, size_t offset,
                     VkDescriptorType type) -> DescriptorWriter&;
    auto writeImage(u32 binding, VkImageView imageView, VkSampler sampler,
                    VkImageLayout layout, VkDescriptorType type)
        -> DescriptorWriter&;

    void updateSet(DeviceContext const& device, VkDescriptorSet set);
    void clear();

   private:
    std::vector<VkWriteDescriptorSet> m_writes;
    std::deque<VkDescriptorImageInfo> m_imageInfos;
    std::deque<VkDescriptorBufferInfo> m_bufferInfos;
};

}  // namespace mpvgl::vlk
