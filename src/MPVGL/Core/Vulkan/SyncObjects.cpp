#include <utility>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/SyncObjects.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

Semaphore::Semaphore(VkSemaphore semaphore, vkb::DispatchTable disp) noexcept
    : m_semaphore(semaphore), m_disp(disp) {}

Semaphore::Semaphore(Semaphore&& other) noexcept
    : m_semaphore(std::exchange(other.m_semaphore, VK_NULL_HANDLE)),
      m_disp(other.m_disp) {}

Semaphore& Semaphore::operator=(Semaphore&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_semaphore = std::exchange(other.m_semaphore, VK_NULL_HANDLE);
        m_disp = other.m_disp;
    }
    return *this;
}

Semaphore::~Semaphore() noexcept { cleanup(); }

void Semaphore::cleanup() noexcept {
    if (m_semaphore != VK_NULL_HANDLE) {
        m_disp.destroySemaphore(m_semaphore, nullptr);
    }
    m_semaphore = VK_NULL_HANDLE;
}

tl::expected<Semaphore, Error<EngineError>> Semaphore::create(
    DeviceContext const& device) {
    auto info = initializers::semaphoreCreateInfo();
    VkSemaphore semaphore = nullptr;
    if (device.logDevDisp.createSemaphore(&info, nullptr, &semaphore) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Semaphore"}};
    }
    return Semaphore(semaphore, device.logDevDisp);
}

Fence::Fence(VkFence fence, vkb::DispatchTable disp) noexcept
    : m_fence(fence), m_disp(disp) {}

Fence::Fence(Fence&& other) noexcept
    : m_fence(std::exchange(other.m_fence, VK_NULL_HANDLE)),
      m_disp(other.m_disp) {}

Fence& Fence::operator=(Fence&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_fence = std::exchange(other.m_fence, VK_NULL_HANDLE);
        m_disp = other.m_disp;
    }
    return *this;
}

Fence::~Fence() noexcept { cleanup(); }

void Fence::cleanup() noexcept {
    if (m_fence != VK_NULL_HANDLE) {
        m_disp.destroyFence(m_fence, nullptr);
    }
    m_fence = VK_NULL_HANDLE;
}

tl::expected<Fence, Error<EngineError>> Fence::create(
    DeviceContext const& device, VkFenceCreateFlags flags) {
    auto info = initializers::fenceCreateInfo(flags);
    VkFence fence = nullptr;
    if (device.logDevDisp.createFence(&info, nullptr, &fence) != VK_SUCCESS) {
        return tl::unexpected{
            Error{EngineError::VulkanRuntimeError, "Failed to create Fence"}};
    }
    return Fence(fence, device.logDevDisp);
}

}  // namespace mpvgl::vlk
