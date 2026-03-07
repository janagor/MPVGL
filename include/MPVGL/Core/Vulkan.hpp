#pragma once

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl::vlk {

tl::expected<void, Error> bootstrap(Vulkan &vulkan);
tl::expected<void, Error> createRenderPass(Vulkan &vulkan);
tl::expected<void, Error> createDescriptorSetLayout(Vulkan &vulkan);
tl::expected<void, Error> createGraphicsPipeline(Vulkan &vulkan);
tl::expected<void, Error> createCommandPool(Vulkan &vulkan);
tl::expected<void, Error> createDepthResources(Vulkan &vulkan);
tl::expected<void, Error> createFramebuffers(Vulkan &vulkan);
tl::expected<void, Error> createTextureImage(Vulkan &vulkan);
tl::expected<void, Error> createTextureImageView(Vulkan &vulkan);
tl::expected<void, Error> createTextureSampler(Vulkan &vulkan);
tl::expected<void, Error> loadModel(Vulkan &vulkan);
tl::expected<void, Error> createVertexBuffer(Vulkan &vulkan);
tl::expected<void, Error> createIndexBuffer(Vulkan &vulkan);
tl::expected<void, Error> createUniformBuffers(Vulkan &vulkan);
tl::expected<void, Error> createDescriptorPool(Vulkan &vulkan);
int create_descriptor_sets(Vulkan &vulkan);
int create_command_buffers(Vulkan &vulkan);
int create_sync_objects(Vulkan &vulkan);
int draw_frame(Vulkan &vulkan);
tl::expected<void, Error> reloadShadersAndPipeline(Vulkan &vulkan);
void cleanup(Vulkan &vulkan);

// TODO: remove from public api
tl::expected<void, Error> deviceInitialization(Vulkan &vulkan);
tl::expected<void, Error> create_swapchain(Vulkan &vulkan);
tl::expected<void, Error> get_queues(Vulkan &vulkan);

}  // namespace mpvgl::vlk
