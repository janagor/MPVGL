#pragma once

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

class CommandPool {
   public:
    CommandPool() = default;
    CommandPool(CommandPool const&) = delete;
    auto operator=(CommandPool const&) -> CommandPool& = delete;
    CommandPool(CommandPool&& other) noexcept;
    auto operator=(CommandPool&& other) noexcept -> CommandPool&;
    ~CommandPool() noexcept;

    [[nodiscard]] static auto create(DeviceContext const& device,
                                     u32 queueFamilyIndex)
        -> tl::expected<CommandPool, Error<EngineError>>;

    [[nodiscard]] auto handle() const noexcept -> VkCommandPool {
        return m_pool;
    }

   private:
    CommandPool(VkCommandPool pool, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

    VkCommandPool m_pool{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;
};

}  // namespace mpvgl::vlk
