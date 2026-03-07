#pragma once

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl::vlk {

tl::expected<void, Error> bootstrap(Vulkan &vulkan);
tl::expected<void, Error> createRenderPass(Vulkan &vulkan);
tl::expected<void, Error> createDescriptorSetLayout(Vulkan &vulkan);
int create_graphics_pipeline(Vulkan &vulkan);
int create_command_pool(Vulkan &vulkan);
int create_depth_resources(Vulkan &vulkan);
int create_framebuffers(Vulkan &vulkan);
int create_texture_image(Vulkan &vulkan);
int create_texture_image_view(Vulkan &vulkan);
int create_texture_sampler(Vulkan &vulkan);
int load_model(Vulkan &vulkan);
int create_vertex_buffer(Vulkan &vulkan);
int create_index_buffer(Vulkan &vulkan);
int create_uniform_buffers(Vulkan &vulkan);
int create_descriptor_pool(Vulkan &vulkan);
int create_descriptor_sets(Vulkan &vulkan);
int create_command_buffers(Vulkan &vulkan);
int create_sync_objects(Vulkan &vulkan);
int draw_frame(Vulkan &vulkan);
int reloadShadersAndPipeline(Vulkan &vulkan);
void cleanup(Vulkan &vulkan);

// TODO: remove from public api
tl::expected<void, Error> deviceInitialization(Vulkan &vulkan);
tl::expected<void, Error> create_swapchain(Vulkan &vulkan);
tl::expected<void, Error> get_queues(Vulkan &vulkan);

}  // namespace mpvgl::vlk
