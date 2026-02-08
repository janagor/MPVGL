#pragma once

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct PhysicalDevice {
    static tl::expected<vkb::PhysicalDevice, std::error_code> getPhysicalDevice(
        vkb::Instance instance, VkSurfaceKHR surface) {
        vkb::PhysicalDeviceSelector phys_device_selector(instance);
        auto phys_device_ret =
            phys_device_selector.set_surface(surface).select();
        if (!phys_device_ret) {
            return tl::unexpected(phys_device_ret.error());
        }
        vkb::PhysicalDevice physical_device = phys_device_ret.value();
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        physical_device.enable_features_if_present(deviceFeatures);
        return physical_device;
    }
};

}  // namespace mpvgl::vlk
