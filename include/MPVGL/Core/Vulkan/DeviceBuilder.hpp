#pragma once

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct DeviceBuilder {
    static tl::expected<vkb::Device, std::error_code> getDevice(
        vkb::PhysicalDevice& physicalDevice) {
        vkb::DeviceBuilder device_builder{physicalDevice};
        auto device = device_builder.build();
        if (!device) return tl::unexpected(device.error());
        return device.value();
    }
};

}  // namespace mpvgl::vlk
