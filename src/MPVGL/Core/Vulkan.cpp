#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define VMA_IMPLEMENTATION
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

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/InstanceBuilder.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"
#include "MPVGL/Core/Vulkan/LogicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/PhysicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Graphics/Color.hpp"

#include "config.hpp"

namespace mpvgl::vlk {

constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

tl::expected<void, Error> deviceInitialization(Vulkan &vulkan) {
    if (auto window = createWindow("Vulkan Triangle", true);
        window.has_value()) {
        vulkan.deviceContext.window = window.value();
    } else {
        return tl::unexpected{Error{window.error()}};
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

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice =
        vulkan.deviceContext.logicalDevice.physical_device;
    allocatorInfo.device = vulkan.deviceContext.logicalDevice.device;
    allocatorInfo.instance = vulkan.deviceContext.instance.instance;
    if (vmaCreateAllocator(&allocatorInfo, &vulkan.deviceContext.allocator) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanInitFailed,
                                    "Failed to create VMA Allocator"}};
    }

    return {};
}

tl::expected<void, Error> create_swapchain(Vulkan &vulkan) {
    auto swapchain = Swapchain::create(vulkan.deviceContext);
    // TODO: Extend the error messages: vkb::Result<Swapchain>.vk_result()
    if (!swapchain) return tl::unexpected(swapchain.error());
    vulkan.swapchainContext.swapchain = std::move(swapchain.value());
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

tl::expected<void, Error> setupRenderingPipeline(Vulkan &vulkan) {
    return createRenderPass(vulkan)
        .and_then([&] { return createDescriptorSetLayout(vulkan); })
        .and_then([&] { return createGraphicsPipeline(vulkan); });
}

tl::expected<void, Error> setupRenderTargets(Vulkan &vulkan) {
    return createDepthResources(vulkan).and_then(
        [&] { return createFramebuffers(vulkan); });
}

tl::expected<void, Error> loadAndPrepareAssets(Vulkan &vulkan) {
    return vlk::createCommandPool(vulkan)
        .and_then([&] { return vlk::loadTexture(vulkan); })
        .and_then([&] { return vlk::loadModel(vulkan); })
        .and_then([&] { return vlk::createVertexBuffer(vulkan); })
        .and_then([&] { return vlk::createIndexBuffer(vulkan); })
        .and_then([&] { return vlk::createUniformBuffers(vulkan); });
}

tl::expected<void, Error> setupDescriptorsAndSync(Vulkan &vulkan) {
    return vlk::createDescriptorPool(vulkan)
        .and_then([&] { return vlk::createDescriptorSets(vulkan); })
        .and_then([&] { return vlk::createCommandBuffers(vulkan); })
        .and_then([&] { return vlk::createSyncObjects(vulkan); });
}

tl::expected<void, Error> createRenderPass(Vulkan &vulkan) {
    VkAttachmentDescription color_attachment = {
        .format = vulkan.swapchainContext.swapchain.format(),
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
    if (auto depthFormat = findDepthFormat(vulkan); depthFormat.has_value()) {
        depth_attachment.format = depthFormat.value();
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    } else {
        return tl::unexpected<Error>{depthFormat.error()};
    }

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
    DescriptorLayoutBuilder builder{};
    return builder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .build(vulkan.deviceContext)
        .transform([&](VkDescriptorSetLayout layout) {
            vulkan.pipelineContext.descriptorSetLayout = layout;
        });
}

tl::expected<void, Error> createGraphicsPipeline(Vulkan &vulkan) {
    auto vertCode =
        readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.vert.spv");
    if (!vertCode.has_value()) return tl::unexpected<Error>{vertCode.error()};
    auto fragCode =
        readFile(std::string(SOURCE_DIRECTORY) + "/shaders/triangle.frag.spv");
    if (!fragCode.has_value()) return tl::unexpected<Error>{fragCode.error()};

    auto vertModule = createShaderModule(vulkan, vertCode.value());
    auto fragModule = createShaderModule(vulkan, fragCode.value());
    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        return tl::unexpected{
            Error{EngineError::ShaderError, "Failed to create Shader Modules"}};
    }

    auto vertStageInfo = initializers::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main");
    auto fragStageInfo = initializers::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main");
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

    vulkan.deviceContext.logDevDisp.destroyShaderModule(fragModule, nullptr);
    vulkan.deviceContext.logDevDisp.destroyShaderModule(vertModule, nullptr);
    return {};
}

tl::expected<void, Error> createFramebuffers(Vulkan &vulkan) {
    auto const &imageViews = vulkan.swapchainContext.swapchain.imageViews();
    vulkan.swapchainContext.framebuffers.resize(
        vulkan.swapchainContext.swapchain.imageViews().size());
    for (size_t i = 0;
         i < vulkan.swapchainContext.swapchain.imageViews().size(); ++i) {
        std::array<VkImageView, 2> attachments = {
            vulkan.swapchainContext.swapchain.imageViews().at(i),
            vulkan.swapchainContext.depthTexture.imageView()};

        auto framebufferInfo = initializers::framebufferCreateInfo(
            vulkan.swapchainContext.renderPass, attachments,
            vulkan.swapchainContext.swapchain.extent(), 1);

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
    return findDepthFormat(vulkan).and_then([&vulkan](VkFormat format) {
        return Texture::createDepthTexture(
                   vulkan.deviceContext,
                   vulkan.swapchainContext.swapchain.extent().width,
                   vulkan.swapchainContext.swapchain.extent().height, format)
            .transform([&vulkan](Texture depthTex) {
                vulkan.swapchainContext.depthTexture = std::move(depthTex);
            });
    });
}

tl::expected<void, Error> loadTexture(Vulkan &vulkan) {
    return Texture::loadFromFile(vulkan.deviceContext, vulkan.data.command_pool,
                                 vulkan.deviceContext.graphicsQueue,
                                 TEXTURE_PATH)
        .transform([&](Texture texture) {
            vulkan.sceneContext.texture = std::move(texture);
        });
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
    return Buffer::createFromData(
               vulkan.deviceContext, vulkan.data.command_pool,
               vulkan.deviceContext.graphicsQueue, vulkan.sceneContext.vertices,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        .transform([&](Buffer buffer) {
            vulkan.sceneContext.vertexBuffer = std::move(buffer);
        });
}

tl::expected<void, Error> createIndexBuffer(Vulkan &vulkan) {
    return Buffer::createFromData(
               vulkan.deviceContext, vulkan.data.command_pool,
               vulkan.deviceContext.graphicsQueue, vulkan.sceneContext.indices,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        .transform([&](Buffer buffer) {
            vulkan.sceneContext.indexBuffer = std::move(buffer);
        });
}

tl::expected<void, Error> createUniformBuffers(Vulkan &vulkan) {
    auto bufferSize = sizeof(UniformBufferObject);
    auto imageCount = vulkan.swapchainContext.swapchain.imageCount();

    vulkan.data.frames.clear();
    vulkan.data.frames.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = {}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto result =
            Buffer::create(
                vulkan.deviceContext, bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .and_then([&](Buffer buffer) {
                    return buffer.map().transform([&](void *mappedData) {
                        vulkan.data.frames.at(i).uniformBufferMapped =
                            mappedData;
                        vulkan.data.frames.at(i).uniformBuffer =
                            std::move(buffer);
                    });
                });
        if (!result.has_value()) {
            return tl::unexpected<Error>{result.error()};
        }
    }
    return {};
}

tl::expected<void, Error> createDescriptorPool(Vulkan &vulkan) {
    std::vector<DescriptorAllocator::PoolSizeRatio> ratios = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0f},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f}};

    return vulkan.data.descriptorAllocator.init(
        vulkan.deviceContext, static_cast<uint32_t>(vulkan.data.frames.size()),
        ratios);
}

tl::expected<void, Error> createDescriptorSets(Vulkan &vulkan) {
    for (size_t i = 0; i < vulkan.data.frames.size(); ++i) {
        auto setRes = vulkan.data.descriptorAllocator.allocate(
            vulkan.deviceContext, vulkan.pipelineContext.descriptorSetLayout);
        if (!setRes) return tl::unexpected(setRes.error());

        vulkan.data.frames.at(i).descriptorSet = setRes.value();

        DescriptorWriter writer{};
        writer
            .writeBuffer(0, vulkan.data.frames.at(i).uniformBuffer.handle(),
                         sizeof(UniformBufferObject), 0,
                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            .writeImage(1, vulkan.sceneContext.texture.imageView(),
                        vulkan.sceneContext.texture.sampler(),
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .updateSet(vulkan.deviceContext,
                       vulkan.data.frames.at(i).descriptorSet);
    }
    return {};
}

tl::expected<void, Error> createCommandBuffers(Vulkan &vulkan) {
    auto framesCount = vulkan.data.frames.size();
    std::vector<VkCommandBuffer> allocatedBuffers(framesCount);

    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(framesCount));

    if (vulkan.deviceContext.logDevDisp.allocateCommandBuffers(
            &allocInfo, allocatedBuffers.data()) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to allocate Command Buffers"}};
    }

    for (size_t i = {}; i < framesCount; ++i) {
        vulkan.data.frames.at(i).commandBuffer = allocatedBuffers.at(i);
    }
    return {};
}

tl::expected<void, Error> createSyncObjects(Vulkan &vulkan) {
    auto imageCount = vulkan.swapchainContext.swapchain.imageCount();
    vulkan.data.image_in_flight.clear();
    vulkan.data.image_in_flight.resize(imageCount, VK_NULL_HANDLE);
    vulkan.data.finishedSemaphores.clear();
    vulkan.data.finishedSemaphores.resize(imageCount, VK_NULL_HANDLE);

    auto semaphoreInfo = initializers::semaphoreCreateInfo();
    auto fenceInfo =
        initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (size_t i = 0; i < imageCount; ++i) {
        if (vulkan.deviceContext.logDevDisp.createSemaphore(
                &semaphoreInfo, nullptr, &vulkan.data.finishedSemaphores[i]) !=
            VK_SUCCESS) {
            return tl::unexpected{
                Error{EngineError::VulkanRuntimeError,
                      "Failed to create Synchronization Objects"}};
        }
    }

    for (size_t i = 0; i < vulkan.data.frames.size(); ++i) {
        if (vulkan.deviceContext.logDevDisp.createSemaphore(
                &semaphoreInfo, nullptr,
                &vulkan.data.frames[i].availableSemaphore) != VK_SUCCESS ||
            vulkan.deviceContext.logDevDisp.createFence(
                &fenceInfo, nullptr, &vulkan.data.frames[i].inFlightFence) !=
                VK_SUCCESS) {
            return tl::unexpected{
                Error{EngineError::VulkanRuntimeError,
                      "Failed to create Synchronization Objects"}};
        }
    }
    return {};
}

tl::expected<void, Error> drawFrame(Vulkan &vulkan) {
    auto &currentFrame = vulkan.data.frames[vulkan.data.current_frame];

    vulkan.deviceContext.logDevDisp.waitForFences(
        1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = vulkan.deviceContext.logDevDisp.acquireNextImageKHR(
        vulkan.swapchainContext.swapchain.handle(), UINT64_MAX,
        currentFrame.availableSemaphore, VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return recreateSwapchain(vulkan);
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to acquire Swapchain Image"}};
    }

    if (vulkan.data.image_in_flight.at(image_index) != VK_NULL_HANDLE) {
        vulkan.deviceContext.logDevDisp.waitForFences(
            1, &vulkan.data.image_in_flight.at(image_index), VK_TRUE,
            UINT64_MAX);
    }
    vulkan.data.image_in_flight.at(image_index) = currentFrame.inFlightFence;

    updateUniformBuffer(vulkan, vulkan.data.current_frame);

    if (auto res = recordCommandBuffer(vulkan, currentFrame.commandBuffer,
                                       image_index);
        !res) {
        return res;
    }

    VkSemaphore finishedSemaphore = vulkan.data.finishedSemaphores[image_index];

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    auto submitInfo = initializers::submitInfo(
        {&currentFrame.availableSemaphore, 1}, {wait_stages, 1},
        {&currentFrame.commandBuffer, 1}, {&finishedSemaphore, 1});

    vulkan.deviceContext.logDevDisp.resetFences(1, &currentFrame.inFlightFence);

    if (vulkan.deviceContext.logDevDisp.queueSubmit(
            vulkan.deviceContext.graphicsQueue, 1, &submitInfo,
            currentFrame.inFlightFence) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to submit Draw Command Buffer"}};
    }

    auto swapchainKRH =
        static_cast<VkSwapchainKHR>(vulkan.swapchainContext.swapchain.handle());
    auto presentInfoKHR = initializers::presentInfoKHR(
        {&finishedSemaphore, 1}, {&swapchainKRH, 1}, {&image_index, 1});

    result = vulkan.deviceContext.logDevDisp.queuePresentKHR(
        vulkan.deviceContext.presentQueue, &presentInfoKHR);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return recreateSwapchain(vulkan);
    } else if (result != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to present Swapchain Image"}};
    }

    vulkan.data.current_frame =
        (vulkan.data.current_frame + 1) % vulkan.data.frames.size();
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
    for (auto &frame : vulkan.data.frames) {
        vulkan.deviceContext.logDevDisp.destroySemaphore(
            frame.availableSemaphore, nullptr);
        vulkan.deviceContext.logDevDisp.destroyFence(frame.inFlightFence,
                                                     nullptr);
    }
    vulkan.data.frames.clear();
    for (auto &sem : vulkan.data.finishedSemaphores) {
        vulkan.deviceContext.logDevDisp.destroySemaphore(sem, nullptr);
    }
    vulkan.data.finishedSemaphores.clear();

    vulkan.deviceContext.logDevDisp.destroyCommandPool(vulkan.data.command_pool,
                                                       nullptr);

    vulkan.deviceContext.logDevDisp.destroyPipeline(
        vulkan.pipelineContext.graphicsPipeline, nullptr);
    vulkan.deviceContext.logDevDisp.destroyPipelineLayout(
        vulkan.pipelineContext.pipelineLayout, nullptr);
    vulkan.deviceContext.logDevDisp.destroyRenderPass(
        vulkan.swapchainContext.renderPass, nullptr);

    vulkan.sceneContext.texture = Texture{};

    vulkan.data.descriptorAllocator.cleanup(vulkan.deviceContext);

    vulkan.deviceContext.logDevDisp.destroyDescriptorSetLayout(
        vulkan.pipelineContext.descriptorSetLayout, nullptr);

    vulkan.sceneContext.vertexBuffer = Buffer{};
    vulkan.sceneContext.indexBuffer = Buffer{};
    vmaDestroyAllocator(vulkan.deviceContext.allocator);

    vkb::destroy_device(vulkan.deviceContext.logicalDevice);
    vkb::destroy_surface(vulkan.deviceContext.instance, vulkan.surface);
    vkb::destroy_instance(vulkan.deviceContext.instance);
    destroy_window_glfw(vulkan.deviceContext.window);
}

}  // namespace mpvgl::vlk
