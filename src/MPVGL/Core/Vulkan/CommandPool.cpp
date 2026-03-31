#include <utility>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/CommandPool.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

CommandPool::CommandPool(VkCommandPool pool, vkb::DispatchTable disp) noexcept
    : m_pool(pool), m_disp(disp) {}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : m_pool(std::exchange(other.m_pool, VK_NULL_HANDLE)),
      m_disp(other.m_disp) {}

auto CommandPool::operator=(CommandPool&& other) noexcept -> CommandPool& {
    if (this != &other) {
        cleanup();
        m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
        m_disp = other.m_disp;
    }
    return *this;
}

CommandPool::~CommandPool() noexcept { cleanup(); }

void CommandPool::cleanup() noexcept {
    if (m_pool != VK_NULL_HANDLE) {
        m_disp.destroyCommandPool(m_pool, nullptr);
    }
    m_pool = VK_NULL_HANDLE;
}

auto CommandPool::create(DeviceContext const& device, u32 queueFamilyIndex)
    -> tl::expected<CommandPool, Error<EngineError>> {
    auto poolInfo = initializers::commandPoolCreateInfo(
        queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandPool pool = nullptr;
    if (device.logDevDisp.createCommandPool(&poolInfo, nullptr, &pool) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Command Pool"}};
    }

    return CommandPool(pool, device.logDevDisp);
}

}  // namespace mpvgl::vlk
