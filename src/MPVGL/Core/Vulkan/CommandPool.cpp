#include "MPVGL/Core/Vulkan/CommandPool.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"

namespace mpvgl::vlk {

CommandPool::CommandPool(VkCommandPool pool, vkb::DispatchTable disp) noexcept
    : m_pool(pool), m_disp(std::move(disp)) {}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : m_pool(other.m_pool), m_disp(std::move(other.m_disp)) {
    other.m_pool = VK_NULL_HANDLE;
}

CommandPool& CommandPool::operator=(CommandPool&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_pool = other.m_pool;
        m_disp = std::move(other.m_disp);

        other.m_pool = VK_NULL_HANDLE;
    }
    return *this;
}

CommandPool::~CommandPool() noexcept { cleanup(); }

void CommandPool::cleanup() noexcept {
    if (m_pool != VK_NULL_HANDLE) {
        m_disp.destroyCommandPool(m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

tl::expected<CommandPool, Error> CommandPool::create(
    DeviceContext const& device, uint32_t queueFamilyIndex) {
    auto poolInfo = initializers::commandPoolCreateInfo(
        queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandPool pool;
    if (device.logDevDisp.createCommandPool(&poolInfo, nullptr, &pool) !=
        VK_SUCCESS) {
        return tl::unexpected(Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Command Pool"});
    }

    return CommandPool(pool, device.logDevDisp);
}

}  // namespace mpvgl::vlk
