#pragma once

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class Semaphore {
   public:
    Semaphore() = default;
    ~Semaphore() noexcept;

    Semaphore(Semaphore const&) = delete;
    Semaphore& operator=(Semaphore const&) = delete;

    Semaphore(Semaphore&& other) noexcept;
    Semaphore& operator=(Semaphore&& other) noexcept;

    [[nodiscard]] static tl::expected<Semaphore, Error<EngineError>> create(
        DeviceContext const& device);

    [[nodiscard]] VkSemaphore handle() const noexcept { return m_semaphore; }

   private:
    Semaphore(VkSemaphore semaphore, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

   private:
    VkSemaphore m_semaphore{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp{};
};

class Fence {
   public:
    Fence() = default;
    ~Fence() noexcept;

    Fence(Fence const&) = delete;
    Fence& operator=(Fence const&) = delete;

    Fence(Fence&& other) noexcept;
    Fence& operator=(Fence&& other) noexcept;

    [[nodiscard]] static tl::expected<Fence, Error<EngineError>> create(
        DeviceContext const& device, VkFenceCreateFlags flags = 0);

    [[nodiscard]] VkFence handle() const noexcept { return m_fence; }

   private:
    Fence(VkFence fence, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

   private:
    VkFence m_fence{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp{};
};

}  // namespace mpvgl::vlk
