#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Device.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Instance.hpp"
#include "MPVGL/Core/Vulkan/PhysicalDevice.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Graphics/Color.hpp"

#include "config.hpp"

namespace mpvgl::vlk {

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
    VkResult err =
        glfwCreateWindowSurface(instance, window, allocator, &surface);
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

tl::expected<void, std::error_code> device_initialization(Vulkan::Init &init) {
    init.window = create_window_glfw("Vulkan Triangle", true);

    auto instance = Instance::getInstance();
    if (!instance) {
        std::cout << instance.error().message() << "\n";
        return tl::unexpected(instance.error());
    }

    init.instance = instance.value();
    init.inst_disp = init.instance.make_table();
    init.surface = create_surface_glfw(init.instance, init.window, nullptr);

    auto phys_device =
        PhysicalDevice::getPhysicalDevice(init.instance, init.surface);
    if (!phys_device) {
        std::cout << phys_device.error().message() << "\n";
        return tl::unexpected(phys_device.error());
    }

    auto device = Device::getDevice(phys_device.value());
    if (!device) {
        std::cout << device.error().message() << "\n";
        return tl::unexpected(device.error());
    }
    init.device = device.value();
    init.disp = init.device.make_table();

    return {};
}

tl::expected<void, std::error_code> create_swapchain(Vulkan::Init &init) {
    auto swapchain =
        Swapchain::getSwapchain(init.device, init.window, init.swapchain);
    if (!swapchain) {
        // TODO: Extend the error messages: vkb::Result<Swapchain>.vk_result()
        std::cout << swapchain.error().message() << "\n";
        return tl::unexpected(swapchain.error());
    }
    init.swapchain = swapchain.value();
    return {};
}

tl::expected<void, std::error_code> get_queues(Vulkan &vulkan) {
    auto gq = vulkan.init.device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value()) {
        std::cout << "failed to get graphics queue: " << gq.error().message()
                  << "\n";
        return tl::unexpected(gq.error());
    }
    vulkan.data.graphics_queue = gq.value();

    auto pq = vulkan.init.device.get_queue(vkb::QueueType::present);
    if (!pq.has_value()) {
        std::cout << "failed to get present queue: " << pq.error().message()
                  << "\n";
        return tl::unexpected(pq.error());
    }
    vulkan.data.present_queue = pq.value();
    return {};
}

int create_render_pass(Vulkan &vulkan) {
    VkAttachmentDescription color_attachment = {
        .format = vulkan.init.swapchain.image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = find_depth_format(vulkan);
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {color_attachment,
                                                          depth_attachment};
    auto render_pass_info = initializers::renderPassCreateInfo(
        attachments, {&subpass, 1}, {&dependency, 1}, 0);

    if (vulkan.init.disp.createRenderPass(&render_pass_info, nullptr,
                                          &vulkan.data.render_pass) !=
        VK_SUCCESS) {
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

// TODO: createShaderModule(Vulkan&vulkan, std::span<const std::uint32_t>
// &code);
VkShaderModule createShaderModule(Vulkan &vulkan,
                                  const std::vector<char> &code) {
    std::span<const std::uint32_t> code_span{
        reinterpret_cast<const std::uint32_t *>(code.data()),
        code.size() / sizeof(std::uint32_t)};
    auto create_info = initializers::shaderModuleCreateInfo(code_span);

    VkShaderModule shaderModule;
    if (vulkan.init.disp.createShaderModule(&create_info, nullptr,
                                            &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;  // failed to create shader module
    }

    return shaderModule;
}

int create_descriptor_set_layout(Vulkan &vulkan) {
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

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        uboLayoutBinding, samplerLayoutBinding};
    auto layoutInfo = initializers::descriptorSetLayoutCreateInfo(bindings);
    if (vkCreateDescriptorSetLayout(vulkan.init.device, &layoutInfo, nullptr,
                                    &vulkan.data.descriptor_set_layout) !=
        VK_SUCCESS) {
        std::cout << "failed to create descriptor set layout!\n";
        return -1;  // failed to create descriptor set layout
    }

    return 0;
}

int create_graphics_pipeline(Vulkan &vulkan) {
    auto vert_code =
        readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.vert.spv");
    auto frag_code =
        readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.frag.spv");

    auto vert_module = createShaderModule(vulkan, vert_code);
    auto frag_module = createShaderModule(vulkan, frag_code);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        std::cout << "failed to create shader module\n";
        return -1;  // failed to create shader modules
    }

    auto vertStageInfo = initializers::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, vert_module, "main");
    auto fragStageInfo = initializers::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, frag_module, "main");
    VkPipelineShaderStageCreateInfo shader_stages[] = {vertStageInfo,
                                                       fragStageInfo};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    auto vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo(
        {&bindingDescription, 1}, attributeDescriptions);

    auto inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

    auto viewport_state = initializers::pipelineViewportStateCreateInfo(1, 1);

    auto rasterizer = initializers::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
        VK_FRONT_FACE_COUNTER_CLOCKWISE);

    auto multisampling =
        initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

    auto depthStencil = initializers::pipelineDepthStencilStateCreateInfo(
        VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    auto colorBlending = initializers::pipelineColorBlendStateCreateInfo(
        {&colorBlendAttachment, 1}, VK_FALSE, VK_LOGIC_OP_COPY);
    auto blendConstants = std::array{0.0f, 0.0f, 0.0f, 0.0f};
    std::copy(blendConstants.begin(), blendConstants.end(),
              colorBlending.blendConstants);

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};
    auto dynamicInfo =
        initializers::pipelineDynamicStateCreateInfo(dynamicStates);

    auto pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(
        {&vulkan.data.descriptor_set_layout, 1}, {});
    if (vkCreatePipelineLayout(vulkan.init.device, &pipelineLayoutInfo, nullptr,
                               &vulkan.data.pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    auto pipelineInfo = initializers::graphicsPipelineCreateInfo();
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shader_stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewport_state;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicInfo;
    pipelineInfo.layout = vulkan.data.pipeline_layout;
    pipelineInfo.renderPass = vulkan.data.render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vulkan.init.disp.createGraphicsPipelines(
            VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
            &vulkan.data.graphics_pipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipline\n";
        return -1;  // failed to create graphics pipeline
    }

    vulkan.init.disp.destroyShaderModule(frag_module, nullptr);
    vulkan.init.disp.destroyShaderModule(vert_module, nullptr);
    return 0;
}

int create_framebuffers(Vulkan &vulkan) {
    vulkan.data.swapchain_images = vulkan.init.swapchain.get_images().value();
    vulkan.data.swapchain_image_views =
        vulkan.init.swapchain.get_image_views().value();

    vulkan.data.framebuffers.resize(vulkan.data.swapchain_image_views.size());
    for (size_t i = 0; i < vulkan.data.swapchain_image_views.size(); ++i) {
        std::array<VkImageView, 2> attachments = {
            vulkan.data.swapchain_image_views.at(i),
            vulkan.data.depth_image_view};

        auto framebufferInfo = initializers::framebufferCreateInfo(
            vulkan.data.render_pass, attachments, vulkan.init.swapchain.extent,
            1);

        if (vulkan.init.disp.createFramebuffer(
                &framebufferInfo, nullptr, &vulkan.data.framebuffers.at(i)) !=
            VK_SUCCESS) {
            return -1;  // failed to create framebuffer
        }
    }
    return 0;
}

int create_command_pool(Vulkan &vulkan) {
    auto poolInfo = initializers::commandPoolCreateInfo(
        vulkan.init.device.get_queue_index(vkb::QueueType::graphics).value(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    if (vulkan.init.disp.createCommandPool(
            &poolInfo, nullptr, &vulkan.data.command_pool) != VK_SUCCESS) {
        std::cout << "failed to create command pool\n";
        return -1;  // failed to create command pool
    }
    return 0;
}

uint32_t find_memory_type(Vulkan &vulkan, uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkan.init.device.physical_device,
                                        &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void create_buffer(Vulkan &vulkan, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory) {
    auto bufferInfo = initializers::bufferCreateInfo(size, usage);
    if (vkCreateBuffer(vulkan.init.device, &bufferInfo, nullptr, &buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkan.init.device, buffer, &memRequirements);

    auto allocInfo = initializers::memoryAllocateInfo(
        memRequirements.size,
        find_memory_type(vulkan, memRequirements.memoryTypeBits, properties));

    if (vkAllocateMemory(vulkan.init.device, &allocInfo, nullptr,
                         &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(vulkan.init.device, buffer, bufferMemory, 0);
}

VkCommandBuffer beginSingleTimeCommands(Vulkan &vulkan) {
    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkan.init.device, &allocInfo, &commandBuffer);

    auto beginInfo = initializers::commandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(Vulkan &vulkan,
                                  VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    auto submitInfo = initializers::submitInfo({}, {}, {&commandBuffer, 1}, {});
    vkQueueSubmit(vulkan.data.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan.data.graphics_queue);

    vkFreeCommandBuffers(vulkan.init.device, vulkan.data.command_pool, 1,
                         &commandBuffer);
}

void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                        VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(vulkan, commandBuffer);
}

void createImage(Vulkan &vulkan, uint32_t width, uint32_t height,
                        VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties, VkImage &image,
                        VkDeviceMemory &imageMemory) {
    auto imageInfo = initializers::imageCreateInfo();
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

    if (vkCreateImage(vulkan.init.device, &imageInfo, nullptr, &image) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkan.init.device, image, &memRequirements);

    auto allocInfo = initializers::memoryAllocateInfo(
        memRequirements.size,
        find_memory_type(vulkan, memRequirements.memoryTypeBits, properties));

    if (vkAllocateMemory(vulkan.init.device, &allocInfo, nullptr,
                         &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    vkBindImageMemory(vulkan.init.device, image, imageMemory, 0);
}

void transition_image_layout(Vulkan &vulkan, VkImage image,
                                    VkFormat format, VkImageLayout oldLayout,
                                    VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);

    auto subresourceRange = VkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    auto barrier = initializers::imageMemoryBarrier(oldLayout, newLayout, image,
                                                    subresourceRange);

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

    endSingleTimeCommands(vulkan, commandBuffer);
}

void copy_buffer_to_image(Vulkan &vulkan, VkBuffer buffer, VkImage image,
                          uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);
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

    endSingleTimeCommands(vulkan, commandBuffer);
}

VkImageView createImageView(Vulkan &vulkan, VkImage image,
                                   VkFormat format,
                                   VkImageAspectFlags aspectFlags) {
    auto subresourceRange = VkImageSubresourceRange{
        .aspectMask = aspectFlags,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    auto viewInfo = initializers::imageViewCreateInfo(
        image, VK_IMAGE_VIEW_TYPE_2D, format, subresourceRange);

    VkImageView imageView;
    if (vkCreateImageView(vulkan.init.device, &viewInfo, nullptr, &imageView) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

int create_image_views(Vulkan &vulkan) {
    vulkan.data.swapchain_image_views.resize(
        vulkan.data.swapchain_images.size());
    for (uint32_t i = 0; i < vulkan.data.swapchain_images.size(); ++i) {
        vulkan.data.swapchain_image_views.at(i) = createImageView(
            vulkan, vulkan.data.swapchain_images.at(i),
            vulkan.init.swapchain.image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    return 0;
}
VkFormat find_supported_format(Vulkan &vulkan,
                                      const std::vector<VkFormat> &candidates,
                                      VkImageTiling tiling,
                                      VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vulkan.init.device.physical_device,
                                            format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}
VkFormat find_depth_format(Vulkan &vulkan) {
    return find_supported_format(
        vulkan,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
bool has_stencil_component(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}

int create_depth_resources(Vulkan &vulkan) {
    VkFormat depthFormat = find_depth_format(vulkan);

    createImage(vulkan, vulkan.init.swapchain.extent.width,
                vulkan.init.swapchain.extent.height, depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.data.depth_image,
                vulkan.data.depth_image_memory);
    vulkan.data.depth_image_view =
        createImageView(vulkan, vulkan.data.depth_image, depthFormat,
                        VK_IMAGE_ASPECT_DEPTH_BIT);
    return 0;
}

int create_texture_image(Vulkan &vulkan) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(TEXTURE_PATH, &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        std::cout << "failed to load texture image\n";
        return -1;  // failed to load texture image
    }
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(vulkan, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);
    void *d;
    vkMapMemory(vulkan.init.device, stagingBufferMemory, 0, imageSize, 0, &d);
    memcpy(d, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(vulkan.init.device, stagingBufferMemory);
    createImage(vulkan, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.data.texture.image,
                vulkan.data.texture.imageMemory);

    transition_image_layout(vulkan, vulkan.data.texture.image,
                            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(vulkan, stagingBuffer, vulkan.data.texture.image,
                         static_cast<uint32_t>(texWidth),
                         static_cast<uint32_t>(texHeight));
    transition_image_layout(vulkan, vulkan.data.texture.image,
                            VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(vulkan.init.device, stagingBuffer, nullptr);
    vkFreeMemory(vulkan.init.device, stagingBufferMemory, nullptr);
    return 0;
}

int create_texture_image_view(Vulkan &vulkan) {
    vulkan.data.texture.imageView =
        createImageView(vulkan, vulkan.data.texture.image,
                        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    return 0;
}

int create_texture_sampler(Vulkan &vulkan) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vulkan.init.device.physical_device,
                                  &properties);

    auto samplerInfo = initializers::samplerCreateInfo();
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
    if (vkCreateSampler(vulkan.init.device, &samplerInfo, nullptr,
                        &vulkan.data.texture.sampler) != VK_SUCCESS) {
        std::cout << "failed to create texture sampler!\n";
        return -1;  // failed to create texture sampler!
    }
    return 0;
}

int load_model(Vulkan &vulkan) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          MODEL_PATH)) {
        throw std::runtime_error(err);
    }
    for (auto const &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            auto vertex =
                Vertex{{attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]},
                       {255, 255, 255},
                       {attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}};
            if (vulkan.data.uniqueVertices.count(vertex) == 0) {
                vulkan.data.uniqueVertices[vertex] =
                    static_cast<uint32_t>(vulkan.data.vertices.size());
                vulkan.data.vertices.push_back(vertex);
            }
            vulkan.data.indices.push_back(
                vulkan.data.uniqueVertices.at(vertex));
        };
    };
    return 0;
}

int create_vertex_buffer(Vulkan &vulkan) {
    VkDeviceSize bufferSize =
        sizeof(vulkan.data.vertices.at(0)) * vulkan.data.vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(vulkan, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void *d;
    vkMapMemory(vulkan.init.device, stagingBufferMemory, 0, bufferSize, 0, &d);
    memcpy(d, vulkan.data.vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(vulkan.init.device, stagingBufferMemory);

    create_buffer(
        vulkan, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.data.vertex_buffer,
        vulkan.data.vertex_buffer_memory);

    copy_buffer(vulkan, stagingBuffer, vulkan.data.vertex_buffer, bufferSize);
    vkDestroyBuffer(vulkan.init.device, stagingBuffer, nullptr);
    vkFreeMemory(vulkan.init.device, stagingBufferMemory, nullptr);
    return 0;
}

int create_index_buffer(Vulkan &vulkan) {
    VkDeviceSize bufferSize =
        sizeof(vulkan.data.indices.at(0)) * vulkan.data.indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(vulkan, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void *d;
    vkMapMemory(vulkan.init.device, stagingBufferMemory, 0, bufferSize, 0, &d);
    memcpy(d, vulkan.data.indices.data(), (size_t)bufferSize);
    vkUnmapMemory(vulkan.init.device, stagingBufferMemory);

    create_buffer(
        vulkan, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.data.index_buffer,
        vulkan.data.index_buffer_memory);

    copy_buffer(vulkan, stagingBuffer, vulkan.data.index_buffer, bufferSize);

    vkDestroyBuffer(vulkan.init.device, stagingBuffer, nullptr);
    vkFreeMemory(vulkan.init.device, stagingBufferMemory, nullptr);
    return 0;
}

int create_uniform_buffers(Vulkan &vulkan) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    vulkan.data.uniform_buffers.resize(vulkan.init.swapchain.image_count);
    vulkan.data.uniform_buffers_memory.resize(
        vulkan.init.swapchain.image_count);
    vulkan.data.uniform_buffers_mapped.resize(
        vulkan.init.swapchain.image_count);

    for (size_t i = 0; i < vulkan.init.swapchain.image_count; ++i) {
        create_buffer(vulkan, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      vulkan.data.uniform_buffers.at(i),
                      vulkan.data.uniform_buffers_memory.at(i));

        vkMapMemory(vulkan.init.device,
                    vulkan.data.uniform_buffers_memory.at(i), 0, bufferSize, 0,
                    &vulkan.data.uniform_buffers_mapped.at(i));
    }
    return 0;
}

int create_descriptor_pool(Vulkan &vulkan) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount =
        static_cast<uint32_t>(vulkan.data.framebuffers.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount =
        static_cast<uint32_t>(vulkan.data.framebuffers.size());

    auto poolInfo = initializers::descriptorPoolCreateInfo(
        poolSizes, vulkan.data.framebuffers.size());

    if (vkCreateDescriptorPool(vulkan.init.device, &poolInfo, nullptr,
                               &vulkan.data.descriptor_pool) != VK_SUCCESS) {
        std::cout << "failed to create descriptor pool\n";
        return -1;
    }
    return 0;
}

int create_descriptor_sets(Vulkan &vulkan) {
    std::vector<VkDescriptorSetLayout> layouts(
        vulkan.data.framebuffers.size(), vulkan.data.descriptor_set_layout);
    auto allocInfo = initializers::descriptorSetAllocateInfo(
        vulkan.data.descriptor_pool, layouts);

    vulkan.data.descriptor_sets.resize(vulkan.data.framebuffers.size());
    if (vkAllocateDescriptorSets(vulkan.init.device, &allocInfo,
                                 vulkan.data.descriptor_sets.data()) !=
        VK_SUCCESS) {
        std::cout << "failed to allocate descriptor sets\n";
        return -1;
    }

    for (size_t i = 0; i < vulkan.init.swapchain.image_count; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vulkan.data.uniform_buffers.at(i);
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vulkan.data.texture.imageView;
        imageInfo.sampler = vulkan.data.texture.sampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{
            initializers::writeDescriptorSet(
                vulkan.data.descriptor_sets.at(i), 0, 0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {&bufferInfo, 1}),
            initializers::writeDescriptorSet(
                vulkan.data.descriptor_sets.at(i), 1, 0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {&imageInfo, 1})};
        vkUpdateDescriptorSets(vulkan.init.device,
                               static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
    return 0;
}

int create_command_buffers(Vulkan &vulkan) {
    vulkan.data.command_buffers.resize(vulkan.data.framebuffers.size());
    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(vulkan.data.command_buffers.size()));
    if (vulkan.init.disp.allocateCommandBuffers(
            &allocInfo, vulkan.data.command_buffers.data()) != VK_SUCCESS) {
        return -1;  // failed to allocate command buffers;
    }
    return 0;
}

int create_sync_objects(Vulkan &vulkan) {
    // TODO:
    vulkan.data.available_semaphores.resize(vulkan.init.swapchain.image_count,
                                            VK_NULL_HANDLE);
    vulkan.data.finished_semaphore.resize(vulkan.init.swapchain.image_count,
                                          VK_NULL_HANDLE);
    vulkan.data.in_flight_fences.resize(vulkan.init.swapchain.image_count,
                                        VK_NULL_HANDLE);
    vulkan.data.image_in_flight.resize(vulkan.init.swapchain.image_count,
                                       VK_NULL_HANDLE);

    auto semaphoreInfo = initializers::semaphoreCreateInfo();
    auto fenceInfo =
        initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (size_t i = 0; i < vulkan.init.swapchain.image_count; ++i) {
        if (vulkan.init.disp.createSemaphore(
                &semaphoreInfo, nullptr,
                &vulkan.data.available_semaphores.at(i)) != VK_SUCCESS ||
            vulkan.init.disp.createSemaphore(
                &semaphoreInfo, nullptr,
                &vulkan.data.finished_semaphore.at(i)) != VK_SUCCESS ||
            vulkan.init.disp.createFence(&fenceInfo, nullptr,
                                         &vulkan.data.in_flight_fences.at(i)) !=
                VK_SUCCESS) {
            std::cout << "failed to create sync objects\n";
            return -1;  // failed to create synchronization objects for a frame
        }
    }
    return 0;
}

void cleanupSwapChain(Vulkan &vulkan) {
    if (vulkan.data.depth_image_view != VK_NULL_HANDLE) {
        vulkan.init.disp.destroyImageView(vulkan.data.depth_image_view,
                                          nullptr);
        vulkan.init.disp.destroyImage(vulkan.data.depth_image, nullptr);
        vulkan.init.disp.freeMemory(vulkan.data.depth_image_memory, nullptr);
    }

    for (auto framebuffer : vulkan.data.framebuffers) {
        vulkan.init.disp.destroyFramebuffer(framebuffer, nullptr);
    }

    vulkan.init.swapchain.destroy_image_views(
        vulkan.data.swapchain_image_views);

    vkb::destroy_swapchain(vulkan.init.swapchain);
}

int recreate_swapchain(Vulkan &vulkan) {
    vulkan.init.disp.deviceWaitIdle();

    if (vulkan.data.depth_image_view != VK_NULL_HANDLE) {
        vulkan.init.disp.destroyImageView(vulkan.data.depth_image_view,
                                          nullptr);
        vulkan.init.disp.destroyImage(vulkan.data.depth_image, nullptr);
        vulkan.init.disp.freeMemory(vulkan.data.depth_image_memory, nullptr);
    }

    vulkan.init.disp.destroyCommandPool(vulkan.data.command_pool, nullptr);
    for (auto framebuffer : vulkan.data.framebuffers) {
        vulkan.init.disp.destroyFramebuffer(framebuffer, nullptr);
    }

    vulkan.init.swapchain.destroy_image_views(
        vulkan.data.swapchain_image_views);
    vulkan.data.framebuffers.clear();
    vulkan.data.swapchain_image_views.clear();

    if (!create_swapchain(vulkan.init).has_value()) return -1;
    vulkan.data.swapchain_images = vulkan.init.swapchain.get_images().value();
    if (0 != create_depth_resources(vulkan)) return -1;
    if (0 != create_image_views(vulkan)) return -1;
    if (0 != create_framebuffers(vulkan)) return -1;
    if (0 != create_command_pool(vulkan)) return -1;
    if (0 != create_command_buffers(vulkan)) return -1;

    return 0;
}

int record_command_buffer(Vulkan &vulkan, VkCommandBuffer command_buffer,
                          uint32_t image_index) {
    auto beginInfo = initializers::commandBufferBeginInfo();
    if (vulkan.init.disp.beginCommandBuffer(command_buffer, &beginInfo) !=
        VK_SUCCESS) {
        return -1;  // failed to begin recording command buffer
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    auto renderPassInfo = initializers::renderPassBeginInfo(
        vulkan.data.render_pass, vulkan.data.framebuffers.at(image_index),
        VkRect2D{VkOffset2D{0, 0}, vulkan.init.swapchain.extent}, clearValues);

    vulkan.init.disp.cmdBeginRenderPass(command_buffer, &renderPassInfo,
                                        VK_SUBPASS_CONTENTS_INLINE);
    vulkan.init.disp.cmdBindPipeline(command_buffer,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     vulkan.data.graphics_pipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkan.init.swapchain.extent.width;
    viewport.height = (float)vulkan.init.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vulkan.init.disp.cmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.init.swapchain.extent;
    vulkan.init.disp.cmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vulkan.data.vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, vulkan.data.index_buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vulkan.data.pipeline_layout, 0, 1,
                            &vulkan.data.descriptor_sets.at(image_index), 0,
                            nullptr);

    vkCmdDrawIndexed(command_buffer,
                     static_cast<uint32_t>(vulkan.data.indices.size()), 1, 0, 0,
                     0);

    vulkan.init.disp.cmdEndRenderPass(command_buffer);

    if (vulkan.init.disp.endCommandBuffer(command_buffer) != VK_SUCCESS) {
        std::cout << "failed to record command buffer\n";
        return -1;  // failed to record command buffer!
    }
    return 0;
}

int update_uniform_buffer(Vulkan &vulkan, uint32_t current_image) {
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
    ubo.projection =
        glm::perspective(glm::radians(45.0f),
                         vulkan.init.swapchain.extent.width /
                             (float)vulkan.init.swapchain.extent.height,
                         0.1f, 10.0f);
    ubo.projection[1][1] *= -1;

    memcpy(vulkan.data.uniform_buffers_mapped.at(current_image), &ubo,
           sizeof(ubo));
    return 0;
}

int draw_frame(Vulkan &vulkan) {
    vulkan.init.disp.waitForFences(
        1, &vulkan.data.in_flight_fences.at(vulkan.data.current_frame), VK_TRUE,
        UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = vulkan.init.disp.acquireNextImageKHR(
        vulkan.init.swapchain, UINT64_MAX,
        vulkan.data.available_semaphores.at(vulkan.data.current_frame),
        VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreate_swapchain(vulkan);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "failed to acquire swapchain image. Error " << result
                  << "\n";
        return -1;
    }

    if (vulkan.data.image_in_flight.at(image_index) != VK_NULL_HANDLE) {
        vulkan.init.disp.waitForFences(
            1, &vulkan.data.image_in_flight.at(image_index), VK_TRUE,
            UINT64_MAX);
    }
    vulkan.data.image_in_flight.at(image_index) =
        vulkan.data.in_flight_fences.at(vulkan.data.current_frame);

    update_uniform_buffer(vulkan, vulkan.data.current_frame);
    record_command_buffer(
        vulkan, vulkan.data.command_buffers.at(vulkan.data.current_frame),
        image_index);

    auto *wait_semaphores =
        &vulkan.data.available_semaphores.at(vulkan.data.current_frame);
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    auto *commandBuffers =
        &vulkan.data.command_buffers.at(vulkan.data.current_frame);
    auto *signal_semaphores = &vulkan.data.finished_semaphore.at(image_index);

    auto submitInfo =
        initializers::submitInfo({wait_semaphores, 1}, {wait_stages, 1},
                                 {commandBuffers, 1}, {signal_semaphores, 1});

    vulkan.init.disp.resetFences(
        1, &vulkan.data.in_flight_fences.at(vulkan.data.current_frame));
    if (vulkan.init.disp.queueSubmit(
            vulkan.data.graphics_queue, 1, &submitInfo,
            vulkan.data.in_flight_fences.at(vulkan.data.current_frame)) !=
        VK_SUCCESS) {
        std::cout << "failed to submit draw command buffer\n";
        return -1;  //"failed to submit draw command buffer
    }

    auto swapchainKRH = static_cast<VkSwapchainKHR>(vulkan.init.swapchain);
    auto presentInfoKHR = initializers::presentInfoKHR(
        {signal_semaphores, 1}, {&swapchainKRH, 1}, {&image_index, 1});

    result = vulkan.init.disp.queuePresentKHR(vulkan.data.present_queue,
                                              &presentInfoKHR);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return recreate_swapchain(vulkan);
    } else if (result != VK_SUCCESS) {
        std::cout << "failed to present swapchain image\n";
        return -1;
    }

    vulkan.data.current_frame =
        (vulkan.data.current_frame + 1) % vulkan.data.framebuffers.size();
    return 0;
}

int reloadShadersAndPipeline(Vulkan &vulkan) {
    vkQueueWaitIdle(vulkan.data.graphics_queue);

    vulkan.init.disp.destroyPipeline(vulkan.data.graphics_pipeline, nullptr);
    vulkan.init.disp.destroyPipelineLayout(vulkan.data.pipeline_layout,
                                           nullptr);

    return create_graphics_pipeline(vulkan);
}

void cleanup(Vulkan &vulkan) {
    cleanupSwapChain(vulkan);

    for (size_t i = 0; i < vulkan.init.swapchain.image_count; ++i) {
        vulkan.init.disp.destroySemaphore(vulkan.data.finished_semaphore.at(i),
                                          nullptr);
        vulkan.init.disp.destroySemaphore(
            vulkan.data.available_semaphores.at(i), nullptr);
        vulkan.init.disp.destroyFence(vulkan.data.in_flight_fences.at(i),
                                      nullptr);
    }

    vulkan.init.disp.destroyCommandPool(vulkan.data.command_pool, nullptr);

    vulkan.init.disp.destroyPipeline(vulkan.data.graphics_pipeline, nullptr);
    vulkan.init.disp.destroyPipelineLayout(vulkan.data.pipeline_layout,
                                           nullptr);
    vulkan.init.disp.destroyRenderPass(vulkan.data.render_pass, nullptr);

    vkDestroyImageView(vulkan.init.device, vulkan.data.texture.imageView,
                       nullptr);
    vkDestroyImage(vulkan.init.device, vulkan.data.texture.image, nullptr);
    vkDestroySampler(vulkan.init.device, vulkan.data.texture.sampler, nullptr);
    vkFreeMemory(vulkan.init.device, vulkan.data.texture.imageMemory, nullptr);

    for (size_t i = 0; i < vulkan.data.framebuffers.size(); i++) {
        vkDestroyBuffer(vulkan.init.device, vulkan.data.uniform_buffers.at(i),
                        nullptr);
        vkFreeMemory(vulkan.init.device,
                     vulkan.data.uniform_buffers_memory.at(i), nullptr);
    }

    vkDestroyDescriptorPool(vulkan.init.device, vulkan.data.descriptor_pool,
                            nullptr);

    vkDestroyDescriptorSetLayout(vulkan.init.device,
                                 vulkan.data.descriptor_set_layout, nullptr);

    vkDestroyBuffer(vulkan.init.device, vulkan.data.vertex_buffer, nullptr);
    vkFreeMemory(vulkan.init.device, vulkan.data.vertex_buffer_memory, nullptr);

    vkDestroyBuffer(vulkan.init.device, vulkan.data.index_buffer, nullptr);
    vkFreeMemory(vulkan.init.device, vulkan.data.index_buffer_memory, nullptr);

    vkb::destroy_device(vulkan.init.device);
    vkb::destroy_surface(vulkan.init.instance, vulkan.init.surface);
    vkb::destroy_instance(vulkan.init.instance);
    destroy_window_glfw(vulkan.init.window);
}

}  // namespace mpvgl::vlk
