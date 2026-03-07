#include "MPVGL/Core/Error.hpp"
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION

#include <cstdint>
#include <cstring>
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
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/InstanceBuilder.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"
#include "MPVGL/Core/Vulkan/LogicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/PhysicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/SwapchainBuilder.hpp"
#include "MPVGL/Graphics/Color.hpp"

#include "config.hpp"

namespace mpvgl::vlk {

tl::expected<void, Error> deviceInitialization(Vulkan &vulkan) {
    if (auto *window = create_window_glfw("Vulkan Triangle", true); window) {
        vulkan.deviceContext.window = window;
    } else {
        return tl::unexpected{
            Error{EngineError::WindowError, "Failed to create GLFW window"}};
    }

    if (auto instance = InstanceBuilder::getInstance(); instance.has_value()) {
        vulkan.deviceContext.instance = instance.value();
    } else {
        return tl::unexpected{
            Error{instance.error(), "Failed to create Instance"}};
    }

    vulkan.deviceContext.instDisp = vulkan.deviceContext.instance.make_table();
    vulkan.surface = create_surface_glfw(vulkan.deviceContext.instance,
                                         vulkan.deviceContext.window, nullptr);

    auto physicalDevice = PhysicalDeviceBuilder::getPhysicalDevice(
        vulkan.deviceContext.instance, vulkan.surface);
    if (!physicalDevice) {
        return tl::unexpected{
            Error{physicalDevice.error(), "Failed to create Physical Device"}};
    }

    if (auto logicalDevice =
            LogicalDeviceBuilder::getLogicalDevice(physicalDevice.value());
        logicalDevice.has_value()) {
        vulkan.deviceContext.logicalDevice = logicalDevice.value();
    } else {
        return tl::unexpected{
            Error{logicalDevice.error(), "Failed to create Logical Device"}};
    }

    vulkan.deviceContext.logDevDisp =
        vulkan.deviceContext.logicalDevice.make_table();

    return {};
}

tl::expected<void, Error> create_swapchain(Vulkan &vulkan) {
    auto swapchain = SwapchainBuilder::getSwapchain(
        vulkan.deviceContext.logicalDevice, vulkan.deviceContext.window,
        vulkan.swapchainContext.swapchain);
    // TODO: Extend the error messages: vkb::Result<Swapchain>.vk_result()
    if (!swapchain) return tl::unexpected(swapchain.error());
    vulkan.swapchainContext.swapchain = swapchain.value();
    return {};
}

tl::expected<void, Error> get_queues(Vulkan &vulkan) {
    if (auto gq = vulkan.deviceContext.logicalDevice.get_queue(
            vkb::QueueType::graphics);
        gq.has_value()) {
        vulkan.deviceContext.graphicsQueue = std::move(gq.value());
    } else {
        return tl::unexpected{
            Error{gq.error(), "Failed to get Graphics Queue"}};
    }

    if (auto pq = vulkan.deviceContext.logicalDevice.get_queue(
            vkb::QueueType::present);
        pq.has_value()) {
        vulkan.deviceContext.presentQueue = std::move(pq.value());
    } else {
        return tl::unexpected(Error{pq.error(), "Failed to get Present Queue"});
    }

    return {};
}

tl::expected<void, Error> bootstrap(Vulkan &vulkan) {
    return deviceInitialization(vulkan)
        .and_then([&] { return create_swapchain(vulkan); })
        .and_then([&] { return get_queues(vulkan); });
}

tl::expected<void, Error> createRenderPass(Vulkan &vulkan) {
    VkAttachmentDescription color_attachment = {
        .format = vulkan.swapchainContext.swapchain.image_format,
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

    if (vulkan.deviceContext.logDevDisp.createRenderPass(
            &render_pass_info, nullptr, &vulkan.swapchainContext.renderPass) !=
        VK_SUCCESS) {
        return tl::unexpected(Error(EngineError::VulkanRuntimeError,
                                    "Failed to create Render Pass"));
    }
    return {};
}

tl::expected<void, Error> createDescriptorSetLayout(Vulkan &vulkan) {
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
    if (vulkan.deviceContext.logDevDisp.createDescriptorSetLayout(
            &layoutInfo, nullptr,
            &vulkan.pipelineContext.descriptorSetLayout) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Descriptor Set Layout"}};
    }

    return {};
}

tl::expected<void, Error> createGraphicsPipeline(Vulkan &vulkan) {
    auto vert_code =
        readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.vert.spv");
    auto frag_code =
        readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.frag.spv");

    auto vert_module = createShaderModule(vulkan, vert_code);
    auto frag_module = createShaderModule(vulkan, frag_code);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        return tl::unexpected{
            Error{EngineError::ShaderError, "Failed to create Shader Modules"}};
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
        {&vulkan.pipelineContext.descriptorSetLayout, 1}, {});
    if (vulkan.deviceContext.logDevDisp.createPipelineLayout(
            &pipelineLayoutInfo, nullptr,
            &vulkan.pipelineContext.pipelineLayout) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Pipeline Layout"}};
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
    pipelineInfo.layout = vulkan.pipelineContext.pipelineLayout;
    pipelineInfo.renderPass = vulkan.swapchainContext.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vulkan.deviceContext.logDevDisp.createGraphicsPipelines(
            VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
            &vulkan.pipelineContext.graphicsPipeline) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Graphics Pipeline"}};
    }

    vulkan.deviceContext.logDevDisp.destroyShaderModule(frag_module, nullptr);
    vulkan.deviceContext.logDevDisp.destroyShaderModule(vert_module, nullptr);
    return {};
}
tl::expected<void, Error> createFramebuffers(Vulkan &vulkan) {
    vulkan.swapchainContext.swapchainImages =
        vulkan.swapchainContext.swapchain.get_images().value();
    vulkan.swapchainContext.swapchainImageViews =
        vulkan.swapchainContext.swapchain.get_image_views().value();

    vulkan.swapchainContext.framebuffers.resize(
        vulkan.swapchainContext.swapchainImageViews.size());
    for (size_t i = 0; i < vulkan.swapchainContext.swapchainImageViews.size();
         ++i) {
        std::array<VkImageView, 2> attachments = {
            vulkan.swapchainContext.swapchainImageViews.at(i),
            vulkan.swapchainContext.depthImageView};

        auto framebufferInfo = initializers::framebufferCreateInfo(
            vulkan.swapchainContext.renderPass, attachments,
            vulkan.swapchainContext.swapchain.extent, 1);

        if (vulkan.deviceContext.logDevDisp.createFramebuffer(
                &framebufferInfo, nullptr,
                &vulkan.swapchainContext.framebuffers.at(i)) != VK_SUCCESS) {
            return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                        "Failed to create Framebuffers"}};
        }
    }
    return {};
}

tl::expected<void, Error> createCommandPool(Vulkan &vulkan) {
    auto poolInfo = initializers::commandPoolCreateInfo(
        vulkan.deviceContext.logicalDevice
            .get_queue_index(vkb::QueueType::graphics)
            .value(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    if (vulkan.deviceContext.logDevDisp.createCommandPool(
            &poolInfo, nullptr, &vulkan.data.command_pool) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Command Pool"}};
    }
    return {};
}

tl::expected<void, Error> createDepthResources(Vulkan &vulkan) {
    VkFormat depthFormat = find_depth_format(vulkan);
    return createImage(vulkan, vulkan.swapchainContext.swapchain.extent.width,
                       vulkan.swapchainContext.swapchain.extent.height, 1,
                       depthFormat, VK_IMAGE_TILING_OPTIMAL,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       vulkan.swapchainContext.depthImage,
                       vulkan.swapchainContext.depthImageMemory)
        .and_then([&vulkan, depthFormat]() -> tl::expected<void, Error> {
            vulkan.swapchainContext.depthImageView =
                createImageView(vulkan, vulkan.swapchainContext.depthImage,
                                depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
            return {};
        });
}

tl::expected<void, Error> createTextureImage(Vulkan &vulkan) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(TEXTURE_PATH, &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);
    vulkan.sceneContext.texture.mipLevels =
        static_cast<uint32_t>(
            std::floor(std::log2(std::max(texWidth, texHeight)))) +
        1;
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to load Texture Image"}};
    }
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(vulkan, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);
    void *d;
    vulkan.deviceContext.logDevDisp.mapMemory(stagingBufferMemory, 0, imageSize,
                                              0, &d);
    memcpy(d, pixels, static_cast<size_t>(imageSize));
    vulkan.deviceContext.logDevDisp.unmapMemory(stagingBufferMemory);
    if (auto result = createImage(
            vulkan, texWidth, texHeight, vulkan.sceneContext.texture.mipLevels,
            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vulkan.sceneContext.texture.image,
            vulkan.sceneContext.texture.imageMemory);
        !result.has_value()) {
        return result;
    }
    transition_image_layout(vulkan, vulkan.sceneContext.texture.image,
                            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            vulkan.sceneContext.texture.mipLevels);
    copy_buffer_to_image(
        vulkan, stagingBuffer, vulkan.sceneContext.texture.image,
        static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    vulkan.deviceContext.logDevDisp.destroyBuffer(stagingBuffer, nullptr);
    vulkan.deviceContext.logDevDisp.freeMemory(stagingBufferMemory, nullptr);

    generateMipmaps(vulkan, vulkan.sceneContext.texture.image,
                    VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight,
                    vulkan.sceneContext.texture.mipLevels);

    return {};
}

tl::expected<void, Error> createTextureImageView(Vulkan &vulkan) {
    vulkan.sceneContext.texture.imageView = createImageView(
        vulkan, vulkan.sceneContext.texture.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT, vulkan.sceneContext.texture.mipLevels);
    return {};
}

tl::expected<void, Error> createTextureSampler(Vulkan &vulkan) {
    VkPhysicalDeviceProperties properties{};
    vulkan.deviceContext.instDisp.getPhysicalDeviceProperties(
        vulkan.deviceContext.logicalDevice.physical_device, &properties);

    auto samplerInfo = initializers::samplerCreateInfo();
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    if (vulkan.deviceContext.logDevDisp.createSampler(
            &samplerInfo, nullptr, &vulkan.sceneContext.texture.sampler) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Texture Sampler"}};
    }
    return {};
}

tl::expected<void, Error> loadModel(Vulkan &vulkan) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          MODEL_PATH)) {
        return tl::unexpected{
            Error{EngineError::VulkanRuntimeError, "Failed to load Model"}};
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
            if (vulkan.sceneContext.uniqueVertices.count(vertex) == 0) {
                vulkan.sceneContext.uniqueVertices[vertex] =
                    static_cast<uint32_t>(vulkan.sceneContext.vertices.size());
                vulkan.sceneContext.vertices.push_back(vertex);
            }
            vulkan.sceneContext.indices.push_back(
                vulkan.sceneContext.uniqueVertices.at(vertex));
        };
    };
    return {};
}

tl::expected<void, Error> createVertexBuffer(Vulkan &vulkan) {
    VkDeviceSize bufferSize = sizeof(vulkan.sceneContext.vertices.at(0)) *
                              vulkan.sceneContext.vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(vulkan, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void *d;
    vulkan.deviceContext.logDevDisp.mapMemory(stagingBufferMemory, 0,
                                              bufferSize, 0, &d);
    memcpy(d, vulkan.sceneContext.vertices.data(),
           static_cast<size_t>(bufferSize));
    vulkan.deviceContext.logDevDisp.unmapMemory(stagingBufferMemory);

    create_buffer(
        vulkan, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.sceneContext.vertexBuffer,
        vulkan.sceneContext.vertexBufferMemory);

    copy_buffer(vulkan, stagingBuffer, vulkan.sceneContext.vertexBuffer,
                bufferSize);
    vulkan.deviceContext.logDevDisp.destroyBuffer(stagingBuffer, nullptr);
    vulkan.deviceContext.logDevDisp.freeMemory(stagingBufferMemory, nullptr);
    return {};
}

tl::expected<void, Error> createIndexBuffer(Vulkan &vulkan) {
    VkDeviceSize bufferSize = sizeof(vulkan.sceneContext.indices.at(0)) *
                              vulkan.sceneContext.indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(vulkan, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void *d;
    vulkan.deviceContext.logDevDisp.mapMemory(stagingBufferMemory, 0,
                                              bufferSize, 0, &d);
    memcpy(d, vulkan.sceneContext.indices.data(), (size_t)bufferSize);
    vulkan.deviceContext.logDevDisp.unmapMemory(stagingBufferMemory);

    create_buffer(
        vulkan, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.sceneContext.indexBuffer,
        vulkan.sceneContext.indexBufferMemory);

    copy_buffer(vulkan, stagingBuffer, vulkan.sceneContext.indexBuffer,
                bufferSize);

    vulkan.deviceContext.logDevDisp.destroyBuffer(stagingBuffer, nullptr);
    vulkan.deviceContext.logDevDisp.freeMemory(stagingBufferMemory, nullptr);
    return {};
}

tl::expected<void, Error> createUniformBuffers(Vulkan &vulkan) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    vulkan.data.uniform_buffers.resize(
        vulkan.swapchainContext.swapchain.image_count);
    vulkan.data.uniform_buffers_memory.resize(
        vulkan.swapchainContext.swapchain.image_count);
    vulkan.data.uniform_buffers_mapped.resize(
        vulkan.swapchainContext.swapchain.image_count);

    for (size_t i = 0; i < vulkan.swapchainContext.swapchain.image_count; ++i) {
        create_buffer(vulkan, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      vulkan.data.uniform_buffers.at(i),
                      vulkan.data.uniform_buffers_memory.at(i));

        vulkan.deviceContext.logDevDisp.mapMemory(
            vulkan.data.uniform_buffers_memory.at(i), 0, bufferSize, 0,
            &vulkan.data.uniform_buffers_mapped.at(i));
    }
    return {};
}

tl::expected<void, Error> createDescriptorPool(Vulkan &vulkan) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount =
        static_cast<uint32_t>(vulkan.swapchainContext.framebuffers.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount =
        static_cast<uint32_t>(vulkan.swapchainContext.framebuffers.size());

    auto poolInfo = initializers::descriptorPoolCreateInfo(
        poolSizes, vulkan.swapchainContext.framebuffers.size());

    if (vulkan.deviceContext.logDevDisp.createDescriptorPool(
            &poolInfo, nullptr, &vulkan.data.descriptor_pool) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Descriptor Pool"}};
    }
    return {};
}

tl::expected<void, Error> createDescriptorSets(Vulkan &vulkan) {
    std::vector<VkDescriptorSetLayout> layouts(
        vulkan.swapchainContext.framebuffers.size(),
        vulkan.pipelineContext.descriptorSetLayout);
    auto allocInfo = initializers::descriptorSetAllocateInfo(
        vulkan.data.descriptor_pool, layouts);

    vulkan.data.descriptor_sets.resize(
        vulkan.swapchainContext.framebuffers.size());
    if (vulkan.deviceContext.logDevDisp.allocateDescriptorSets(
            &allocInfo, vulkan.data.descriptor_sets.data()) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to allocate Descriptor Sets"}};
    }

    for (size_t i = 0; i < vulkan.swapchainContext.swapchain.image_count; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vulkan.data.uniform_buffers.at(i);
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vulkan.sceneContext.texture.imageView;
        imageInfo.sampler = vulkan.sceneContext.texture.sampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{
            initializers::writeDescriptorSet(
                vulkan.data.descriptor_sets.at(i), 0, 0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {&bufferInfo, 1}),
            initializers::writeDescriptorSet(
                vulkan.data.descriptor_sets.at(i), 1, 0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {&imageInfo, 1})};
        vulkan.deviceContext.logDevDisp.updateDescriptorSets(
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(), 0, nullptr);
    }
    return {};
}

tl::expected<void, Error> createCommandBuffers(Vulkan &vulkan) {
    vulkan.data.command_buffers.resize(
        vulkan.swapchainContext.framebuffers.size());
    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(vulkan.data.command_buffers.size()));
    if (vulkan.deviceContext.logDevDisp.allocateCommandBuffers(
            &allocInfo, vulkan.data.command_buffers.data()) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to allocate Command Buffers"}};
    }
    return {};
}

tl::expected<void, Error> createSyncObjects(Vulkan &vulkan) {
    // TODO:
    vulkan.data.available_semaphores.resize(
        vulkan.swapchainContext.swapchain.image_count, VK_NULL_HANDLE);
    vulkan.data.finished_semaphore.resize(
        vulkan.swapchainContext.swapchain.image_count, VK_NULL_HANDLE);
    vulkan.data.in_flight_fences.resize(
        vulkan.swapchainContext.swapchain.image_count, VK_NULL_HANDLE);
    vulkan.data.image_in_flight.resize(
        vulkan.swapchainContext.swapchain.image_count, VK_NULL_HANDLE);

    auto semaphoreInfo = initializers::semaphoreCreateInfo();
    auto fenceInfo =
        initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (size_t i = 0; i < vulkan.swapchainContext.swapchain.image_count; ++i) {
        if (vulkan.deviceContext.logDevDisp.createSemaphore(
                &semaphoreInfo, nullptr,
                &vulkan.data.available_semaphores.at(i)) != VK_SUCCESS ||
            vulkan.deviceContext.logDevDisp.createSemaphore(
                &semaphoreInfo, nullptr,
                &vulkan.data.finished_semaphore.at(i)) != VK_SUCCESS ||
            vulkan.deviceContext.logDevDisp.createFence(
                &fenceInfo, nullptr, &vulkan.data.in_flight_fences.at(i)) !=
                VK_SUCCESS) {
            return tl::unexpected{
                Error{EngineError::VulkanRuntimeError,
                      "Failed to create Synchronization Objects for a Frame"}};
        }
    }
    return {};
}

tl::expected<void, Error> drawFrame(Vulkan &vulkan) {
    vulkan.deviceContext.logDevDisp.waitForFences(
        1, &vulkan.data.in_flight_fences.at(vulkan.data.current_frame), VK_TRUE,
        UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = vulkan.deviceContext.logDevDisp.acquireNextImageKHR(
        vulkan.swapchainContext.swapchain, UINT64_MAX,
        vulkan.data.available_semaphores.at(vulkan.data.current_frame),
        VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        auto res = recreate_swapchain(vulkan);
        return tl::unexpected{Error{EngineError::Unknown,
                                    "Failed to draw frame because of "
                                    "`VK_ERROR_OUT_OF_DATE_KHR`. Error " +
                                        std::to_string(res)}};
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return tl::unexpected{
            Error{EngineError::VulkanRuntimeError,
                  "Failed to acquire Swapchain Image. Error " +
                      std::to_string(result)}};
    }

    if (vulkan.data.image_in_flight.at(image_index) != VK_NULL_HANDLE) {
        vulkan.deviceContext.logDevDisp.waitForFences(
            1, &vulkan.data.image_in_flight.at(image_index), VK_TRUE,
            UINT64_MAX);
    }
    vulkan.data.image_in_flight.at(image_index) =
        vulkan.data.in_flight_fences.at(vulkan.data.current_frame);

    update_uniform_buffer(vulkan, vulkan.data.current_frame);
    if (auto result = recordCommandBuffer(
            vulkan, vulkan.data.command_buffers.at(vulkan.data.current_frame),
            image_index);
        !result.has_value()) {
        return result;
        ;
    }

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

    vulkan.deviceContext.logDevDisp.resetFences(
        1, &vulkan.data.in_flight_fences.at(vulkan.data.current_frame));
    if (vulkan.deviceContext.logDevDisp.queueSubmit(
            vulkan.deviceContext.graphicsQueue, 1, &submitInfo,
            vulkan.data.in_flight_fences.at(vulkan.data.current_frame)) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to submit Draw Command Buffer"}};
    }

    auto swapchainKRH =
        static_cast<VkSwapchainKHR>(vulkan.swapchainContext.swapchain);
    auto presentInfoKHR = initializers::presentInfoKHR(
        {signal_semaphores, 1}, {&swapchainKRH, 1}, {&image_index, 1});

    result = vulkan.deviceContext.logDevDisp.queuePresentKHR(
        vulkan.deviceContext.presentQueue, &presentInfoKHR);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        auto res = recreate_swapchain(vulkan);
        return tl::unexpected{
            Error{EngineError::Unknown,
                  "Failed to draw frame because of "
                  "`VK_ERROR_OUT_OF_DATE_KHR` or `VK_SUBOPTIMAL_KHR`."
                  " Error " +
                      std::to_string(res)}};
    } else if (result != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to present Swapchain Image"}};
    }

    vulkan.data.current_frame = (vulkan.data.current_frame + 1) %
                                vulkan.swapchainContext.framebuffers.size();
    return {};
}

tl::expected<void, Error> reloadShadersAndPipeline(Vulkan &vulkan) {
    vulkan.deviceContext.logDevDisp.queueWaitIdle(
        vulkan.deviceContext.graphicsQueue);

    vulkan.deviceContext.logDevDisp.destroyPipeline(
        vulkan.pipelineContext.graphicsPipeline, nullptr);
    vulkan.deviceContext.logDevDisp.destroyPipelineLayout(
        vulkan.pipelineContext.pipelineLayout, nullptr);

    return createGraphicsPipeline(vulkan);
}

void cleanup(Vulkan &vulkan) {
    cleanupSwapChain(vulkan);

    for (size_t i = 0; i < vulkan.swapchainContext.swapchain.image_count; ++i) {
        vulkan.deviceContext.logDevDisp.destroySemaphore(
            vulkan.data.finished_semaphore.at(i), nullptr);
        vulkan.deviceContext.logDevDisp.destroySemaphore(
            vulkan.data.available_semaphores.at(i), nullptr);
        vulkan.deviceContext.logDevDisp.destroyFence(
            vulkan.data.in_flight_fences.at(i), nullptr);
    }

    vulkan.deviceContext.logDevDisp.destroyCommandPool(vulkan.data.command_pool,
                                                       nullptr);

    vulkan.deviceContext.logDevDisp.destroyPipeline(
        vulkan.pipelineContext.graphicsPipeline, nullptr);
    vulkan.deviceContext.logDevDisp.destroyPipelineLayout(
        vulkan.pipelineContext.pipelineLayout, nullptr);
    vulkan.deviceContext.logDevDisp.destroyRenderPass(
        vulkan.swapchainContext.renderPass, nullptr);

    vulkan.deviceContext.logDevDisp.destroyImageView(
        vulkan.sceneContext.texture.imageView, nullptr);
    vulkan.deviceContext.logDevDisp.destroyImage(
        vulkan.sceneContext.texture.image, nullptr);
    vulkan.deviceContext.logDevDisp.destroySampler(
        vulkan.sceneContext.texture.sampler, nullptr);
    vulkan.deviceContext.logDevDisp.freeMemory(
        vulkan.sceneContext.texture.imageMemory, nullptr);

    for (size_t i = 0; i < vulkan.swapchainContext.framebuffers.size(); i++) {
        vulkan.deviceContext.logDevDisp.destroyBuffer(
            vulkan.data.uniform_buffers.at(i), nullptr);
        vulkan.deviceContext.logDevDisp.freeMemory(
            vulkan.data.uniform_buffers_memory.at(i), nullptr);
    }

    vulkan.deviceContext.logDevDisp.destroyDescriptorPool(
        vulkan.data.descriptor_pool, nullptr);

    vulkan.deviceContext.logDevDisp.destroyDescriptorSetLayout(
        vulkan.pipelineContext.descriptorSetLayout, nullptr);

    vulkan.deviceContext.logDevDisp.destroyBuffer(
        vulkan.sceneContext.vertexBuffer, nullptr);
    vulkan.deviceContext.logDevDisp.freeMemory(
        vulkan.sceneContext.vertexBufferMemory, nullptr);

    vulkan.deviceContext.logDevDisp.destroyBuffer(
        vulkan.sceneContext.indexBuffer, nullptr);
    vulkan.deviceContext.logDevDisp.freeMemory(
        vulkan.sceneContext.indexBufferMemory, nullptr);

    vkb::destroy_device(vulkan.deviceContext.logicalDevice);
    vkb::destroy_surface(vulkan.deviceContext.instance, vulkan.surface);
    vkb::destroy_instance(vulkan.deviceContext.instance);
    destroy_window_glfw(vulkan.deviceContext.window);
}

}  // namespace mpvgl::vlk
