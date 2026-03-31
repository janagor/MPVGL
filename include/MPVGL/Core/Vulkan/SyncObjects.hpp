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
    auto operator=(Semaphore const&) -> Semaphore& = delete;

    Semaphore(Semaphore&& other) noexcept;
    auto operator=(Semaphore&& other) noexcept -> Semaphore&;

    [[nodiscard]] static auto create(DeviceContext const& device)
        -> tl::expected<Semaphore, Error<EngineError>>;

    [[nodiscard]] auto handle() const noexcept -> VkSemaphore {
        return m_semaphore;
    }

   private:
    Semaphore(VkSemaphore semaphore, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

    VkSemaphore m_semaphore{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;
};

class Fence {
   public:
    Fence() = default;
    ~Fence() noexcept;

    Fence(Fence const&) = delete;
    auto operator=(Fence const&) -> Fence& = delete;

    Fence(Fence&& other) noexcept;
    auto operator=(Fence&& other) noexcept -> Fence&;

    [[nodiscard]] static auto create(DeviceContext const& device,
                                     VkFenceCreateFlags flags = 0)
        -> tl::expected<Fence, Error<EngineError>>;

    [[nodiscard]] auto handle() const noexcept -> VkFence { return m_fence; }

   private:
    Fence(VkFence fence, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

    VkFence m_fence{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;
};

}  // namespace mpvgl::vlk
