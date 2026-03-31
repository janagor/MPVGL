#pragma once

#include <system_error>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct LogicalDeviceBuilder {
    static auto getLogicalDevice(vkb::PhysicalDevice& physicalDevice)
        -> tl::expected<vkb::Device, std::error_code> {
        vkb::DeviceBuilder const logicalDeviceBuilder{physicalDevice};
        auto logicalDevice = logicalDeviceBuilder.build();
        if (!logicalDevice) {
            return tl::unexpected{logicalDevice.error()};
        };
        return logicalDevice.value();
    }
};

}  // namespace mpvgl::vlk
