#pragma once

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class CommandPool {
   public:
    CommandPool() = default;
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&& other) noexcept;
    CommandPool& operator=(CommandPool&& other) noexcept;
    ~CommandPool() noexcept;

    [[nodiscard]] static tl::expected<CommandPool, Error<EngineError>> create(
        DeviceContext const& device, uint32_t queueFamilyIndex);

    [[nodiscard]] VkCommandPool handle() const noexcept { return m_pool; }

   private:
    CommandPool(VkCommandPool pool, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

   private:
    VkCommandPool m_pool{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp{};
};

}  // namespace mpvgl::vlk
