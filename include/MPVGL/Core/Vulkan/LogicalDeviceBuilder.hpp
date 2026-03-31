#pragma once

#include <system_error>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct LogicalDeviceBuilder {
    static tl::expected<vkb::Device, std::error_code> getLogicalDevice(
        vkb::PhysicalDevice& physicalDevice) {
        vkb::DeviceBuilder const logicalDeviceBuilder{physicalDevice};
        auto logicalDevice = logicalDeviceBuilder.build();
        if (!logicalDevice) {
            return tl::unexpected{logicalDevice.error()};
        };
        return logicalDevice.value();
    }
};

}  // namespace mpvgl::vlk
