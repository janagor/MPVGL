#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/Descriptor.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

DescriptorLayoutBuilder& DescriptorLayoutBuilder::addBinding(
    uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding = binding;
    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = nullptr;
    newBinding.stageFlags = stageFlags;
    m_bindings.push_back(newBinding);
    return *this;
}

void DescriptorLayoutBuilder::clear() { m_bindings.clear(); }

tl::expected<VkDescriptorSetLayout, Error<EngineError>>
DescriptorLayoutBuilder::build(DeviceContext const& device) {
    auto info = initializers::descriptorSetLayoutCreateInfo(m_bindings);
    VkDescriptorSetLayout layout;
    if (device.logDevDisp.createDescriptorSetLayout(&info, nullptr, &layout) !=
        VK_SUCCESS) {
        return tl::unexpected<Error<EngineError>>{
            EngineError::VulkanRuntimeError,
            "Failed to create Descriptor Set Layout"};
    }
    return layout;
}

tl::expected<void, Error<EngineError>> DescriptorAllocator::init(
    DeviceContext const& device, uint32_t maxSets,
    std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (auto ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = static_cast<uint32_t>(ratio.ratio * maxSets)});
    }

    auto poolInfo = initializers::descriptorPoolCreateInfo(poolSizes, maxSets);
    if (device.logDevDisp.createDescriptorPool(&poolInfo, nullptr, &m_pool) !=
        VK_SUCCESS) {
        return tl::unexpected(Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Descriptor Pool"});
    }
    return {};
}

void DescriptorAllocator::cleanup(DeviceContext const& device) {
    if (m_pool != VK_NULL_HANDLE) {
        device.logDevDisp.destroyDescriptorPool(m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

tl::expected<VkDescriptorSet, Error<EngineError>> DescriptorAllocator::allocate(
    DeviceContext const& device, VkDescriptorSetLayout layout) {
    auto allocInfo =
        initializers::descriptorSetAllocateInfo(m_pool, {&layout, 1});
    VkDescriptorSet set;
    if (device.logDevDisp.allocateDescriptorSets(&allocInfo, &set) !=
        VK_SUCCESS) {
        return tl::unexpected<Error<EngineError>>{
            EngineError::VulkanRuntimeError,
            "Failed to allocate Descriptor Set"};
    }
    return set;
}

DescriptorWriter& DescriptorWriter::writeBuffer(uint32_t binding,
                                                VkBuffer buffer, size_t size,
                                                size_t offset,
                                                VkDescriptorType type) {
    auto& info = m_bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer, .offset = offset, .range = size});

    VkWriteDescriptorSet write = initializers::writeDescriptorSet(
        VK_NULL_HANDLE, binding, 0, type,
        std::span<VkDescriptorBufferInfo const>{&info, 1});
    m_writes.push_back(write);
    return *this;
}

DescriptorWriter& DescriptorWriter::writeImage(uint32_t binding,
                                               VkImageView imageView,
                                               VkSampler sampler,
                                               VkImageLayout layout,
                                               VkDescriptorType type) {
    auto& info = m_imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler, .imageView = imageView, .imageLayout = layout});

    VkWriteDescriptorSet write = initializers::writeDescriptorSet(
        VK_NULL_HANDLE, binding, 0, type,
        std::span<VkDescriptorImageInfo const>{&info, 1});
    m_writes.push_back(write);
    return *this;
}

void DescriptorWriter::updateSet(DeviceContext const& device,
                                 VkDescriptorSet set) {
    for (auto& write : m_writes) {
        write.dstSet = set;
    }
    device.logDevDisp.updateDescriptorSets(
        static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);
}

void DescriptorWriter::clear() {
    m_imageInfos.clear();
    m_bufferInfos.clear();
    m_writes.clear();
}

}  // namespace mpvgl::vlk
