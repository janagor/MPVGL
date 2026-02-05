#define GLM_FORCE_RADIANS
// clang-format off
#include <chrono>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

#include <glm/glm.hpp>
#include <VkBootstrap.h>
#include <tl/expected.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/IO/PPMLoader.hpp"
#include "MPVGL/Graphics/Color.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/UniformBufferObject.hpp"

#include "config.hpp"
// clang-format on

namespace mpvgl {

namespace {

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, Color::literal::Red, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, Color::literal::Green, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, Color::literal::Blue, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, Color::literal::White, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, Color::literal::Red, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, Color::literal::Green, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, Color::literal::Blue, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, Color::literal::White, {0.0f, 1.0f}},
};

// clang-format off
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
// clang-format on

};  // namespace

GLFWwindow *create_window_glfw(const char *window_name, bool resize) {
  glfwSetErrorCallback([](int error, const char *description) {
    fprintf(stderr, "GLFW Error (%d): %s\n", error, description);
  });
  if (glfwInit()) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // if (!resize) glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    return glfwCreateWindow(800, 600, window_name, NULL, NULL);
  }
  throw std::runtime_error("Nie udało się zainicjować GLFW!");
}

void destroy_window_glfw(GLFWwindow *window) {
  glfwDestroyWindow(window);
  glfwTerminate();
}

VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow *window,
                                 VkAllocationCallbacks *allocator) {
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkResult err = glfwCreateWindowSurface(instance, window, allocator, &surface);
  if (err) {
    const char *error_msg;
    int ret = glfwGetError(&error_msg);
    if (ret != 0) {
      std::cout << ret << " ";
      if (error_msg != nullptr) std::cout << error_msg;
      std::cout << "\n";
    }
    surface = VK_NULL_HANDLE;
  }
  return surface;
}

tl::expected<void, std::error_code> device_initialization(Init &init) {
  init.window = create_window_glfw("Vulkan Triangle", true);

  vkb::InstanceBuilder instance_builder;
  auto instance_ret = instance_builder.use_default_debug_messenger()
                          .request_validation_layers()
                          .build();
  if (!instance_ret) {
    std::cout << instance_ret.error().message() << "\n";
    return tl::unexpected(instance_ret.error());
  }
  init.instance = instance_ret.value();

  init.inst_disp = init.instance.make_table();

  init.surface = create_surface_glfw(init.instance, init.window);

  vkb::PhysicalDeviceSelector phys_device_selector(init.instance);
  auto phys_device_ret =
      phys_device_selector.set_surface(init.surface).select();
  if (!phys_device_ret) {
    std::cout << phys_device_ret.error().message() << "\n";
    return tl::unexpected(phys_device_ret.error());
  }
  vkb::PhysicalDevice physical_device = phys_device_ret.value();

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  physical_device.enable_features_if_present(deviceFeatures);

  vkb::DeviceBuilder device_builder{physical_device};
  auto device_ret = device_builder.build();
  if (!device_ret) {
    std::cout << device_ret.error().message() << "\n";
    return tl::unexpected(device_ret.error());
  }
  init.device = device_ret.value();

  init.disp = init.device.make_table();

  return {};
}

tl::expected<void, std::error_code> create_swapchain(Init &init) {
  vkb::SwapchainBuilder swapchain_builder{init.device};
  int width, height;
  glfwGetWindowSize(init.window, &width, &height);
  auto swap_ret = swapchain_builder
                      .set_desired_extent(static_cast<uint32_t>(width),
                                          static_cast<uint32_t>(height))
                      .set_old_swapchain(init.swapchain)
                      .build();
  if (!swap_ret) {
    std::cout << swap_ret.error().message() << " " << swap_ret.vk_result()
              << "\n";
    return tl::unexpected(swap_ret.error());
  }
  vkb::destroy_swapchain(init.swapchain);
  init.swapchain = swap_ret.value();
  return {};
}

tl::expected<void, std::error_code> get_queues(Init &init, RenderData &data) {
  auto gq = init.device.get_queue(vkb::QueueType::graphics);
  if (!gq.has_value()) {
    std::cout << "failed to get graphics queue: " << gq.error().message()
              << "\n";
    return tl::unexpected(gq.error());
  }
  data.graphics_queue = gq.value();

  auto pq = init.device.get_queue(vkb::QueueType::present);
  if (!pq.has_value()) {
    std::cout << "failed to get present queue: " << pq.error().message()
              << "\n";
    return tl::unexpected(pq.error());
  }
  data.present_queue = pq.value();
  return {};
}

int create_render_pass(Init &init, RenderData &data) {
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = init.swapchain.image_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  if (init.disp.createRenderPass(&render_pass_info, nullptr,
                                 &data.render_pass) != VK_SUCCESS) {
    std::cout << "failed to create render pass\n";
    return -1;  // failed to create render pass!
  }
  return 0;
}

std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t file_size = (size_t)file.tellg();
  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(file_size));

  file.close();

  return buffer;
}

VkShaderModule createShaderModule(Init &init, const std::vector<char> &code) {
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (init.disp.createShaderModule(&create_info, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;  // failed to create shader module
  }

  return shaderModule;
}

int create_descriptor_set_layout(Init &init, RenderData &data) {
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                          samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(init.device, &layoutInfo, nullptr,
                                  &data.descriptor_set_layout) != VK_SUCCESS) {
    std::cout << "failed to create descriptor set layout!\n";
    return -1;  // failed to create descriptor set layout
  }

  return 0;
}

int create_graphics_pipeline(Init &init, RenderData &data) {
  auto vert_code =
      readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.vert.spv");
  auto frag_code =
      readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.frag.spv");

  auto vert_module = createShaderModule(init, vert_code);
  auto frag_module = createShaderModule(init, frag_code);
  if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
    std::cout << "failed to create shader module\n";
    return -1;  // failed to create shader modules
  }

  VkPipelineShaderStageCreateInfo vert_stage_info = {};
  vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_stage_info.module = vert_module;
  vert_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_stage_info = {};
  frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_stage_info.module = frag_module;
  frag_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info,
                                                     frag_stage_info};

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
  vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &colorBlendAttachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  // if (init.disp.createPipelineLayout(&pipeline_layout_info, nullptr,
  //                                    &data.pipeline_layout) != VK_SUCCESS) {
  //   std::cout << "failed to create pipeline layout\n";
  //   return -1;  // failed to create pipeline layout
  // }

  std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamic_info = {};
  dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_info.pDynamicStates = dynamic_states.data();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &data.descriptor_set_layout;
  if (vkCreatePipelineLayout(init.device, &pipelineLayoutInfo, nullptr,
                             &data.pipeline_layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_info;
  pipeline_info.layout = data.pipeline_layout;
  pipeline_info.renderPass = data.render_pass;
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  if (init.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info,
                                        nullptr, &data.graphics_pipeline) !=
      VK_SUCCESS) {
    std::cout << "failed to create pipline\n";
    return -1;  // failed to create graphics pipeline
  }

  init.disp.destroyShaderModule(frag_module, nullptr);
  init.disp.destroyShaderModule(vert_module, nullptr);
  return 0;
}

int create_framebuffers(Init &init, RenderData &data) {
  data.swapchain_images = init.swapchain.get_images().value();
  data.swapchain_image_views = init.swapchain.get_image_views().value();

  data.framebuffers.resize(data.swapchain_image_views.size());
  for (size_t i = 0; i < data.swapchain_image_views.size(); ++i) {
    VkImageView attachments[] = {data.swapchain_image_views.at(i)};

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = data.render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = init.swapchain.extent.width;
    framebuffer_info.height = init.swapchain.extent.height;
    framebuffer_info.layers = 1;

    if (init.disp.createFramebuffer(&framebuffer_info, nullptr,
                                    &data.framebuffers.at(i)) != VK_SUCCESS) {
      return -1;  // failed to create framebuffer
    }
  }
  return 0;
}

int create_command_pool(Init &init, RenderData &data) {
  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex =
      init.device.get_queue_index(vkb::QueueType::graphics).value();

  if (init.disp.createCommandPool(&pool_info, nullptr, &data.command_pool) !=
      VK_SUCCESS) {
    std::cout << "failed to create command pool\n";
    return -1;  // failed to create command pool
  }
  return 0;
}

static uint32_t find_memory_type(Init &init, RenderData &data,
                                 uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(init.device.physical_device,
                                      &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

static void create_buffer(Init &init, RenderData &data, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(init.device, &bufferInfo, nullptr, &buffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(init.device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      find_memory_type(init, data, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(init.device, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(init.device, buffer, bufferMemory, 0);
}

static VkCommandBuffer beginSingleTimeCommands(Init &init, RenderData &data) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = data.command_pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(init.device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

static void endSingleTimeCommands(Init &init, RenderData &data,
                                  VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(data.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(data.graphics_queue);

  vkFreeCommandBuffers(init.device, data.command_pool, 1, &commandBuffer);
}

static void copy_buffer(Init &init, RenderData &data, VkBuffer srcBuffer,
                        VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(init, data);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(init, data, commandBuffer);
}

static void createImage(Init &init, RenderData &data, uint32_t width,
                        uint32_t height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties, VkImage &image,
                        VkDeviceMemory &imageMemory) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(init.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(init.device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      find_memory_type(init, data, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(init.device, &allocInfo, nullptr, &imageMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(init.device, image, imageMemory, 0);
}

static void transition_image_layout(Init &init, RenderData &data, VkImage image,
                                    VkFormat format, VkImageLayout oldLayout,
                                    VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(init, data);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(init, data, commandBuffer);
}

void copy_buffer_to_image(Init &init, RenderData &data, VkBuffer buffer,
                          VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(init, data);
  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};
  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(init, data, commandBuffer);
}

int create_texture_image(Init &init, RenderData &data) {
  uint32_t texWidth, texHeight, texChannels;
  auto rgb_pixels = PPMLoader::load("textures/output.ppm");
  auto rgba_pixels = static_cast<Pixel::Map<Pixel::RGBA>>(rgb_pixels);
  auto pixels_data = rgba_pixels.dataPtr();
  texWidth = rgba_pixels.width();
  texHeight = rgba_pixels.height();
  texChannels = rgba_pixels.channels();
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  if (!pixels_data) {
    std::cout << "failed to load texture image\n";
    return -1;  // failed to load texture image
  }
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  create_buffer(init, data, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
  void *d;
  vkMapMemory(init.device, stagingBufferMemory, 0, imageSize, 0, &d);
  memcpy(d, pixels_data, static_cast<size_t>(imageSize));
  vkUnmapMemory(init.device, stagingBufferMemory);
  createImage(init, data, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, data.texture_image,
              data.texture_image_memory);

  transition_image_layout(init, data, data.texture_image,
                          VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copy_buffer_to_image(init, data, stagingBuffer, data.texture_image,
                       static_cast<uint32_t>(texWidth),
                       static_cast<uint32_t>(texHeight));
  transition_image_layout(init, data, data.texture_image,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkDestroyBuffer(init.device, stagingBuffer, nullptr);
  vkFreeMemory(init.device, stagingBufferMemory, nullptr);
  return 0;
}

static VkImageView createImageView(Init &init, RenderData &data, VkImage image,
                                   VkFormat format) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(init.device, &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image view!");
  }

  return imageView;
}

static int create_image_views(Init &init, RenderData &data) {
  data.swapchain_image_views.resize(data.swapchain_images.size());
  for (uint32_t i = 0; i < data.swapchain_images.size(); ++i) {
    data.swapchain_image_views.at(i) = createImageView(
        init, data, data.swapchain_images.at(i), init.swapchain.image_format);
  }
  return 0;
}
int create_texture_image_view(Init &init, RenderData &data) {
  data.texture_image_view =
      createImageView(init, data, data.texture_image, VK_FORMAT_R8G8B8A8_SRGB);
  return 0;
}

int create_texture_sampler(Init &init, RenderData &data) {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(init.device.physical_device, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  // samplerInfo.mipLodBias = 0.0f;
  // samplerInfo.minLod = 0.0f;
  // samplerInfo.maxLod = 0.0f;
  if (vkCreateSampler(init.device, &samplerInfo, nullptr,
                      &data.texture_sampler) != VK_SUCCESS) {
    std::cout << "failed to create texture sampler!\n";
    return -1;  // failed to create texture sampler!
  }
  return 0;
}

int create_vertex_buffer(Init &init, RenderData &data) {
  VkDeviceSize bufferSize = sizeof(vertices.at(0)) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  create_buffer(init, data, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);

  void *d;
  vkMapMemory(init.device, stagingBufferMemory, 0, bufferSize, 0, &d);
  memcpy(d, vertices.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(init.device, stagingBufferMemory);

  create_buffer(
      init, data, bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, data.vertex_buffer,
      data.vertex_buffer_memory);

  copy_buffer(init, data, stagingBuffer, data.vertex_buffer, bufferSize);
  vkDestroyBuffer(init.device, stagingBuffer, nullptr);
  vkFreeMemory(init.device, stagingBufferMemory, nullptr);
  return 0;
}

int create_index_buffer(Init &init, RenderData &data) {
  VkDeviceSize bufferSize = sizeof(indices.at(0)) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  create_buffer(init, data, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);

  void *d;
  vkMapMemory(init.device, stagingBufferMemory, 0, bufferSize, 0, &d);
  memcpy(d, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(init.device, stagingBufferMemory);

  create_buffer(
      init, data, bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, data.index_buffer,
      data.index_buffer_memory);

  copy_buffer(init, data, stagingBuffer, data.index_buffer, bufferSize);

  vkDestroyBuffer(init.device, stagingBuffer, nullptr);
  vkFreeMemory(init.device, stagingBufferMemory, nullptr);
  return 0;
}

int create_uniform_buffers(Init &init, RenderData &data) {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  data.uniform_buffers.resize(init.swapchain.image_count);
  data.uniform_buffers_memory.resize(init.swapchain.image_count);
  data.uniform_buffers_mapped.resize(init.swapchain.image_count);

  for (size_t i = 0; i < init.swapchain.image_count; ++i) {
    create_buffer(init, data, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  data.uniform_buffers.at(i),
                  data.uniform_buffers_memory.at(i));

    vkMapMemory(init.device, data.uniform_buffers_memory.at(i), 0, bufferSize,
                0, &data.uniform_buffers_mapped.at(i));
  }
  return 0;
}

int create_descriptor_pool(Init &init, RenderData &data) {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(data.framebuffers.size());
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(data.framebuffers.size());

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();

  poolInfo.maxSets = static_cast<uint32_t>(data.framebuffers.size());

  if (vkCreateDescriptorPool(init.device, &poolInfo, nullptr,
                             &data.descriptor_pool) != VK_SUCCESS) {
    std::cout << "failed to create descriptor pool\n";
    return -1;
  }
  return 0;
}

int create_descriptor_sets(Init &init, RenderData &data) {
  std::vector<VkDescriptorSetLayout> layouts(data.framebuffers.size(),
                                             data.descriptor_set_layout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = data.descriptor_pool;
  allocInfo.descriptorSetCount =
      static_cast<uint32_t>(data.framebuffers.size());
  allocInfo.pSetLayouts = layouts.data();

  data.descriptor_sets.resize(data.framebuffers.size());
  if (vkAllocateDescriptorSets(init.device, &allocInfo,
                               data.descriptor_sets.data()) != VK_SUCCESS) {
    std::cout << "failed to allocate descriptor sets\n";
    return -1;
  }

  for (size_t i = 0; i < init.swapchain.image_count; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = data.uniform_buffers.at(i);
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = data.texture_image_view;
    imageInfo.sampler = data.texture_sampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = data.descriptor_sets.at(i);
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = data.descriptor_sets.at(i);
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(init.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }

  return 0;
}

int create_command_buffers(Init &init, RenderData &data) {
  data.command_buffers.resize(data.framebuffers.size());

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = data.command_pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)data.command_buffers.size();

  if (init.disp.allocateCommandBuffers(
          &allocInfo, data.command_buffers.data()) != VK_SUCCESS) {
    return -1;  // failed to allocate command buffers;
  }

  return 0;
}

int create_sync_objects(Init &init, RenderData &data) {
  // TODO:
  data.available_semaphores.resize(init.swapchain.image_count, VK_NULL_HANDLE);
  data.finished_semaphore.resize(init.swapchain.image_count, VK_NULL_HANDLE);
  data.in_flight_fences.resize(init.swapchain.image_count, VK_NULL_HANDLE);
  data.image_in_flight.resize(init.swapchain.image_count, VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  // clang-format off
  for (size_t i = 0; i < init.swapchain.image_count; ++i) {
    if (init.disp.createSemaphore(
          &semaphore_info, nullptr, &data.available_semaphores.at(i)) != VK_SUCCESS ||
        init.disp.createSemaphore(
          &semaphore_info, nullptr, &data.finished_semaphore.at(i)) != VK_SUCCESS ||
        init.disp.createFence(
          &fence_info, nullptr, &data.in_flight_fences.at(i)) != VK_SUCCESS) {
      std::cout << "failed to create sync objects\n";
      return -1; // failed to create synchronization objects for a frame
    }
  }
  // clang-format on
  return 0;
}

int recreate_swapchain(Init &init, RenderData &data) {
  init.disp.deviceWaitIdle();

  init.disp.destroyCommandPool(data.command_pool, nullptr);
  for (auto framebuffer : data.framebuffers) {
    init.disp.destroyFramebuffer(framebuffer, nullptr);
  }
  init.swapchain.destroy_image_views(data.swapchain_image_views);
  data.framebuffers.clear();
  data.swapchain_image_views.clear();

  if (!create_swapchain(init).has_value()) return -1;
  if (0 != create_image_views(init, data)) return -1;
  if (0 != create_framebuffers(init, data)) return -1;
  if (0 != create_command_pool(init, data)) return -1;
  if (0 != create_command_buffers(init, data)) return -1;
  return 0;
}

int record_command_buffer(Init &init, RenderData &data,
                          VkCommandBuffer command_buffer,
                          uint32_t image_index) {
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (init.disp.beginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
    return -1;  // failed to begin recording command buffer
  }

  VkRenderPassBeginInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = data.render_pass;
  render_pass_info.framebuffer = data.framebuffers.at(image_index);
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = init.swapchain.extent;

  VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clearColor;

  init.disp.cmdBeginRenderPass(command_buffer, &render_pass_info,
                               VK_SUBPASS_CONTENTS_INLINE);
  init.disp.cmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            data.graphics_pipeline);

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)init.swapchain.extent.width;
  viewport.height = (float)init.swapchain.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  init.disp.cmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = init.swapchain.extent;
  init.disp.cmdSetScissor(command_buffer, 0, 1, &scissor);

  VkBuffer vertexBuffers[] = {data.vertex_buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(command_buffer, data.index_buffer, 0,
                       VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          data.pipeline_layout, 0, 1,
                          &data.descriptor_sets.at(image_index), 0, nullptr);

  vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0,
                   0, 0);

  init.disp.cmdEndRenderPass(command_buffer);

  if (init.disp.endCommandBuffer(command_buffer) != VK_SUCCESS) {
    std::cout << "failed to record command buffer\n";
    return -1;  // failed to record command buffer!
  }
  return 0;
}

int update_uniform_buffer(Init &init, RenderData &data,
                          uint32_t current_image) {
  static auto start_time = std::chrono::high_resolution_clock::now();

  auto current_time = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   current_time - start_time)
                   .count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view =
      glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.projection = glm::perspective(
      glm::radians(45.0f),
      init.swapchain.extent.width / (float)init.swapchain.extent.height, 0.1f,
      10.0f);
  ubo.projection[1][1] *= -1;

  memcpy(data.uniform_buffers_mapped.at(current_image), &ubo, sizeof(ubo));
  return 0;
}

int draw_frame(Init &init, RenderData &data) {
  init.disp.waitForFences(1, &data.in_flight_fences.at(data.current_frame),
                          VK_TRUE, UINT64_MAX);

  uint32_t image_index = 0;
  VkResult result = init.disp.acquireNextImageKHR(
      init.swapchain, UINT64_MAX,
      data.available_semaphores.at(data.current_frame), VK_NULL_HANDLE,
      &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return recreate_swapchain(init, data);
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    std::cout << "failed to acquire swapchain image. Error " << result << "\n";
    return -1;
  }

  if (data.image_in_flight.at(image_index) != VK_NULL_HANDLE) {
    init.disp.waitForFences(1, &data.image_in_flight.at(image_index), VK_TRUE,
                            UINT64_MAX);
  }
  data.image_in_flight.at(image_index) =
      data.in_flight_fences.at(data.current_frame);

  update_uniform_buffer(init, data, data.current_frame);
  record_command_buffer(init, data, data.command_buffers.at(data.current_frame),
                        image_index);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {
      data.available_semaphores.at(data.current_frame)};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = wait_semaphores;
  submitInfo.pWaitDstStageMask = wait_stages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &data.command_buffers.at(data.current_frame);

  VkSemaphore signal_semaphores[] = {data.finished_semaphore.at(image_index)};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signal_semaphores;

  init.disp.resetFences(1, &data.in_flight_fences.at(data.current_frame));
  if (init.disp.queueSubmit(data.graphics_queue, 1, &submitInfo,
                            data.in_flight_fences.at(data.current_frame)) !=
      VK_SUCCESS) {
    std::cout << "failed to submit draw command buffer\n";
    return -1;  //"failed to submit draw command buffer
  }

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;

  VkSwapchainKHR swapChains[] = {init.swapchain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapChains;

  present_info.pImageIndices = &image_index;

  result = init.disp.queuePresentKHR(data.present_queue, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return recreate_swapchain(init, data);
  } else if (result != VK_SUCCESS) {
    std::cout << "failed to present swapchain image\n";
    return -1;
  }

  data.current_frame = (data.current_frame + 1) % data.framebuffers.size();
  return 0;
}

int reloadShadersAndPipeline(Init &init, RenderData &data) {
  vkQueueWaitIdle(data.graphics_queue);

  init.disp.destroyPipeline(data.graphics_pipeline, nullptr);
  init.disp.destroyPipelineLayout(data.pipeline_layout, nullptr);

  return create_graphics_pipeline(init, data);
}

void cleanup(Init &init, RenderData &data) {
  for (size_t i = 0; i < init.swapchain.image_count; ++i) {
    init.disp.destroySemaphore(data.finished_semaphore.at(i), nullptr);
    init.disp.destroySemaphore(data.available_semaphores.at(i), nullptr);
    init.disp.destroyFence(data.in_flight_fences.at(i), nullptr);
  }

  init.disp.destroyCommandPool(data.command_pool, nullptr);

  for (auto framebuffer : data.framebuffers) {
    init.disp.destroyFramebuffer(framebuffer, nullptr);
  }

  init.disp.destroyPipeline(data.graphics_pipeline, nullptr);
  init.disp.destroyPipelineLayout(data.pipeline_layout, nullptr);
  init.disp.destroyRenderPass(data.render_pass, nullptr);

  init.swapchain.destroy_image_views(data.swapchain_image_views);

  vkb::destroy_swapchain(init.swapchain);

  vkDestroyImageView(init.device, data.texture_image_view, nullptr);
  vkDestroyImage(init.device, data.texture_image, nullptr);
  vkDestroySampler(init.device, data.texture_sampler, nullptr);
  vkFreeMemory(init.device, data.texture_image_memory, nullptr);

  for (size_t i = 0; i < data.framebuffers.size(); i++) {
    vkDestroyBuffer(init.device, data.uniform_buffers.at(i), nullptr);
    vkFreeMemory(init.device, data.uniform_buffers_memory.at(i), nullptr);
  }

  vkDestroyDescriptorPool(init.device, data.descriptor_pool, nullptr);

  vkDestroyDescriptorSetLayout(init.device, data.descriptor_set_layout,
                               nullptr);

  vkDestroyBuffer(init.device, data.vertex_buffer, nullptr);
  vkFreeMemory(init.device, data.vertex_buffer_memory, nullptr);

  vkDestroyBuffer(init.device, data.index_buffer, nullptr);
  vkFreeMemory(init.device, data.index_buffer_memory, nullptr);

  vkb::destroy_device(init.device);
  vkb::destroy_surface(init.instance, init.surface);
  vkb::destroy_instance(init.instance);
  destroy_window_glfw(init.window);
}

}  // namespace mpvgl
