#pragma once

#include <iostream>
#include <system_error>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct Instance {
    static tl::expected<vkb::Instance, std::error_code> getInstance() {
        vkb::InstanceBuilder instance_builder;
        auto instance_ret = instance_builder.use_default_debug_messenger()
                                .request_validation_layers()
                                .build();
        if (instance_ret) {
            return instance_ret.value();
        }
        return tl::unexpected(instance_ret.error());
    }
};

};  // namespace mpvgl::vlk
