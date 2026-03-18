#define GLM_FORCE_RADIANS
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/Descriptor.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/InstanceBuilder.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"
#include "MPVGL/Core/Vulkan/LogicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/Model.hpp"
#include "MPVGL/Core/Vulkan/PhysicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/RenderPass.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Geometry/Shape.hpp"
#include "MPVGL/Geometry/ShapeTraits.hpp"

#include "config.hpp"

namespace {

template <typename T>
tl::expected<T, mpvgl::Error<mpvgl::EngineError>> vkb_to_expected(
    vkb::Result<T> &&res, std::string const &msg) {
    if (!res)
        return tl::unexpected{
            mpvgl::Error<mpvgl::EngineError>{res.error(), msg}};
    return std::move(res.value());
}

}  // namespace

namespace mpvgl::vlk {

constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

tl::expected<void, Error<EngineError>> deviceInitialization(Vulkan &vulkan) {
    auto &surface = vulkan.surface;
    auto &deviceContext = vulkan.deviceContext;

    return createWindow("Vulkan Triangle", true)
        .transform_error([](auto e) { return Error{e}; })
        .and_then([&](auto window) {
            deviceContext.window = window;

            return InstanceBuilder::getInstance().transform_error([](auto e) {
                return Error<EngineError>{e, "Failed to create Instance"};
            });
        })
        .and_then([&](auto instance) {
            deviceContext.instance = instance;
            deviceContext.instDisp = deviceContext.instance.make_table();
            surface = create_surface_glfw(deviceContext.instance,
                                          deviceContext.window, nullptr);

            return PhysicalDeviceBuilder::getPhysicalDevice(
                       deviceContext.instance, surface)
                .transform_error([](auto e) {
                    return Error<EngineError>{
                        e, "Failed to create Physical Device"};
                });
        })
        .and_then([](auto physicalDevice) {
            return LogicalDeviceBuilder::getLogicalDevice(physicalDevice)
                .transform_error([](auto e) {
                    return Error<EngineError>{
                        e, "Failed to create Logical Device"};
                });
        })
        .and_then([&](auto logicalDevice)
                      -> tl::expected<void, Error<EngineError>> {
            deviceContext.logicalDevice = logicalDevice;
            deviceContext.logDevDisp = deviceContext.logicalDevice.make_table();

            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice =
                deviceContext.logicalDevice.physical_device;
            allocatorInfo.device = deviceContext.logicalDevice.device;
            allocatorInfo.instance = deviceContext.instance.instance;

            if (vmaCreateAllocator(&allocatorInfo, &deviceContext.allocator) !=
                VK_SUCCESS) {
                return tl::unexpected{Error{EngineError::VulkanInitFailed,
                                            "Failed to create VMA Allocator"}};
            }

            return {};
        });
}

tl::expected<void, Error<EngineError>> createSwapchain(Vulkan &vulkan) {
    return Swapchain::create(vulkan.deviceContext)
        .transform([&vulkan](Swapchain swapchain) {
            vulkan.swapchainContext.swapchain = std::move(swapchain);
        });
}

tl::expected<void, Error<EngineError>> getQueues(Vulkan &vulkan) {
    auto &logicalDevice = vulkan.deviceContext.logicalDevice;
    auto &deviceContext = vulkan.deviceContext;

    return vkb_to_expected(logicalDevice.get_queue(vkb::QueueType::graphics),
                           "Failed to get Graphics Queue")
        .and_then([&deviceContext, &logicalDevice](VkQueue gQueue) {
            deviceContext.graphicsQueue = gQueue;
            return vkb_to_expected(
                logicalDevice.get_queue(vkb::QueueType::present),
                "Failed to get Present Queue");
        })
        .transform(
            [&](VkQueue pQueue) { deviceContext.presentQueue = pQueue; });
}

tl::expected<void, Error<EngineError>> bootstrap(Vulkan &vulkan) {
    return deviceInitialization(vulkan)
        .and_then([&] { return createSwapchain(vulkan); })
        .and_then([&] { return getQueues(vulkan); });
}

tl::expected<void, Error<EngineError>> setupRenderingPipeline(Vulkan &vulkan) {
    return createRenderPass(vulkan)
        .and_then([&] { return createDescriptorSetLayout(vulkan); })
        .and_then([&] { return createGraphicsPipeline(vulkan); });
}

tl::expected<void, Error<EngineError>> setupRenderTargets(Vulkan &vulkan) {
    return createDepthResources(vulkan).and_then(
        [&] { return createFramebuffers(vulkan); });
}

tl::expected<void, Error<EngineError>> loadAndPrepareAssets(
    Vulkan &vulkan, Scene const &scene) {
    return vlk::createCommandPool(vulkan)
        .and_then([&] { return vlk::loadTexture(vulkan); })
        .and_then([&] { return vlk::loadScene(vulkan, scene); })
        .and_then([&] { return vlk::createUniformBuffers(vulkan); });
}

tl::expected<void, Error<EngineError>> setupDescriptorsAndSync(Vulkan &vulkan) {
    return vlk::createDescriptorPool(vulkan)
        .and_then([&] { return vlk::createDescriptorSets(vulkan); })
        .and_then([&] { return vlk::createCommandBuffers(vulkan); })
        .and_then([&] { return vlk::createSyncObjects(vulkan); });
}

tl::expected<void, Error<EngineError>> createRenderPass(Vulkan &vulkan) {
    RenderPassBuilder builder{};

    VkAttachmentDescription colorAttachment = {
        .format = vulkan.swapchainContext.swapchain.format(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    builder.addAttachment(colorAttachment);

    SubpassInfo mainSubpass{};
    mainSubpass.colorAttachments.push_back(
        {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    if (auto depthFormat = findDepthFormat(vulkan); depthFormat.has_value()) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat.value();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        builder.addAttachment(depthAttachment);
        mainSubpass.depthAttachment = VkAttachmentReference{
            1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    } else {
        return tl::unexpected{depthFormat.error()};
    }

    builder.addSubpass(mainSubpass);

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

    builder.addDependency(dependency);

    return builder.build(vulkan.deviceContext)
        .transform([&](RenderPass renderPass) {
            vulkan.swapchainContext.renderPass = std::move(renderPass);
        });
}

tl::expected<void, Error<EngineError>> createDescriptorSetLayout(
    Vulkan &vulkan) {
    DescriptorLayoutBuilder builder{};

    builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                       VK_SHADER_STAGE_VERTEX_BIT);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_FRAGMENT_BIT);

    return builder.build(vulkan.deviceContext)
        .transform([&](DescriptorSetLayout layout) {
            vulkan.pipelineContext.descriptorSetLayout = std::move(layout);
        });
}

tl::expected<void, Error<EngineError>> createGraphicsPipeline(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    auto &pipelineContext = vulkan.pipelineContext;
    auto &swapchainContext = vulkan.swapchainContext;

    return GraphicsPipeline::create(
               deviceContext, swapchainContext.renderPass.handle(),
               pipelineContext.descriptorSetLayout.handle(),
               std::string(SOURCE_DIRECTORY) + "/shaders/triangle.vert.spv",
               std::string(SOURCE_DIRECTORY) + "/shaders/triangle.frag.spv")
        .transform([&](GraphicsPipeline pipeline) {
            pipelineContext.graphicsPipeline = std::move(pipeline);
        });
}

tl::expected<void, Error<EngineError>> createFramebuffers(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    auto &swapchainContext = vulkan.swapchainContext;

    auto const &imageViews = vulkan.swapchainContext.swapchain.imageViews();

    swapchainContext.framebuffers.resize(
        swapchainContext.swapchain.imageViews().size());
    for (size_t i = 0; i < swapchainContext.swapchain.imageViews().size();
         ++i) {
        std::array<VkImageView, 2> attachments = {
            swapchainContext.swapchain.imageViews().at(i),
            swapchainContext.depthTexture.imageView()};

        auto framebufferInfo = initializers::framebufferCreateInfo(
            swapchainContext.renderPass.handle(), attachments,
            swapchainContext.swapchain.extent(), 1);

        if (deviceContext.logDevDisp.createFramebuffer(
                &framebufferInfo, nullptr,
                &swapchainContext.framebuffers.at(i)) != VK_SUCCESS) {
            return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                        "Failed to create Framebuffers"}};
        }
    }
    return {};
}

tl::expected<void, Error<EngineError>> createCommandPool(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    return vkb_to_expected(deviceContext.logicalDevice.get_queue_index(
                               vkb::QueueType::graphics),
                           "Failed to get Queue Index")
        .and_then([&](uint32_t queueIndex) {
            return CommandPool::create(deviceContext, queueIndex)
                .transform([&vulkan](CommandPool pool) {
                    vulkan.data.commandPool = std::move(pool);
                });
        });
}

tl::expected<void, Error<EngineError>> createDepthResources(Vulkan &vulkan) {
    auto &swapchainContext = vulkan.swapchainContext;
    auto &deviceContext = vulkan.deviceContext;

    return findDepthFormat(vulkan).and_then([&](VkFormat format) {
        return Texture::createDepthTexture(
                   deviceContext, swapchainContext.swapchain.extent().width,
                   swapchainContext.swapchain.extent().height, format)
            .transform([&](Texture depthTex) {
                swapchainContext.depthTexture = std::move(depthTex);
            });
    });
}

tl::expected<void, Error<EngineError>> loadTexture(Vulkan &vulkan) {
    auto &sceneContext = vulkan.sceneContext;
    auto &deviceContext = vulkan.deviceContext;

    return Texture::loadFromFile(deviceContext,
                                 vulkan.data.commandPool.handle(),
                                 deviceContext.graphicsQueue, TEXTURE_PATH)
        .transform([&](Texture texture) {
            sceneContext.texture = std::move(texture);
        });
}

tl::expected<void, Error<EngineError>> loadScene(Vulkan &vulkan,
                                                 Scene const &scene) {
    auto &sceneContext = vulkan.sceneContext;
    auto &deviceContext = vulkan.deviceContext;

    for (auto const &node : scene.nodes) {
        auto modelRes = Model::create(
            deviceContext, vulkan.data.commandPool.handle(),
            deviceContext.graphicsQueue, node.mesh.vertices, node.mesh.indices);
        if (!modelRes) return tl::unexpected{modelRes.error()};
        sceneContext.models.emplace_back(std::move(modelRes.value()));
        RenderObject obj{};
        obj.model = &sceneContext.models.back();
        obj.texture = &sceneContext.texture;
        obj.transformMatrix = node.transform;

        sceneContext.renderables.push_back(obj);
    }
    return {};
}

tl::expected<void, Error<EngineError>> createUniformBuffers(Vulkan &vulkan) {
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
            return tl::unexpected{result.error()};
        }
    }
    return {};
}

tl::expected<void, Error<EngineError>> createDescriptorPool(Vulkan &vulkan) {
    std::vector<DescriptorAllocator::PoolSizeRatio> ratios = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0f},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f}};

    return vulkan.data.descriptorAllocator.init(
        vulkan.deviceContext, static_cast<uint32_t>(vulkan.data.frames.size()),
        ratios);
}

tl::expected<void, Error<EngineError>> createDescriptorSets(Vulkan &vulkan) {
    auto &sceneContext = vulkan.sceneContext;
    auto &deviceContext = vulkan.deviceContext;
    auto &pipelineContext = vulkan.pipelineContext;

    for (size_t i = 0; i < vulkan.data.frames.size(); ++i) {
        auto setRes = vulkan.data.descriptorAllocator.allocate(
            deviceContext, pipelineContext.descriptorSetLayout.handle());
        if (!setRes) return tl::unexpected{setRes.error()};

        vulkan.data.frames.at(i).descriptorSet = setRes.value();

        DescriptorWriter writer{};
        writer
            .writeBuffer(0, vulkan.data.frames.at(i).uniformBuffer.handle(),
                         sizeof(UniformBufferObject), 0,
                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            .writeImage(1, sceneContext.texture.imageView(),
                        sceneContext.texture.sampler(),
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .updateSet(deviceContext, vulkan.data.frames.at(i).descriptorSet);
    }
    return {};
}

tl::expected<void, Error<EngineError>> createCommandBuffers(Vulkan &vulkan) {
    auto framesCount = vulkan.data.frames.size();
    std::vector<VkCommandBuffer> allocatedBuffers(framesCount);

    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.commandPool.handle(), VK_COMMAND_BUFFER_LEVEL_PRIMARY,
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

tl::expected<void, Error<EngineError>> createSyncObjects(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    auto imageCount = vulkan.swapchainContext.swapchain.imageCount();

    vulkan.data.imageInFlight.assign(imageCount, VK_NULL_HANDLE);
    vulkan.data.finishedSemaphores.clear();

    for (size_t i = 0; i < imageCount; ++i) {
        if (auto sem = Semaphore::create(deviceContext); sem.has_value()) {
            vulkan.data.finishedSemaphores.push_back(std::move(sem.value()));
        } else {
            return tl::unexpected{sem.error()};
        }
    }

    for (size_t i = 0; i < vulkan.data.frames.size(); ++i) {
        auto semRes = Semaphore::create(deviceContext);
        if (!semRes) return tl::unexpected{semRes.error()};
        vulkan.data.frames[i].availableSemaphore = std::move(semRes.value());

        auto fenceRes =
            Fence::create(deviceContext, VK_FENCE_CREATE_SIGNALED_BIT);
        if (!fenceRes) return tl::unexpected{fenceRes.error()};
        vulkan.data.frames[i].inFlightFence = std::move(fenceRes.value());
    }
    return {};
}

tl::expected<void, Error<EngineError>> drawFrame(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    auto &swapchainContext = vulkan.swapchainContext;

    auto &currentFrame = vulkan.data.frames[vulkan.data.currentFrame];

    VkFence inFlightFence = currentFrame.inFlightFence.handle();
    VkSemaphore availableSemaphore = currentFrame.availableSemaphore.handle();

    deviceContext.logDevDisp.waitForFences(1, &inFlightFence, VK_TRUE,
                                           UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = deviceContext.logDevDisp.acquireNextImageKHR(
        swapchainContext.swapchain.handle(), UINT64_MAX, availableSemaphore,
        VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return recreateSwapchain(vulkan);
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to acquire Swapchain Image"}};
    }

    if (vulkan.data.imageInFlight.at(image_index) != VK_NULL_HANDLE) {
        deviceContext.logDevDisp.waitForFences(
            1, &vulkan.data.imageInFlight.at(image_index), VK_TRUE, UINT64_MAX);
    }
    vulkan.data.imageInFlight.at(image_index) = inFlightFence;

    updateUniformBuffer(vulkan, vulkan.data.currentFrame);

    if (auto res = recordCommandBuffer(vulkan, currentFrame.commandBuffer,
                                       image_index);
        !res) {
        return res;
    }

    VkSemaphore finishedSemaphore =
        vulkan.data.finishedSemaphores[image_index].handle();

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    auto submitInfo = initializers::submitInfo(
        {&availableSemaphore, 1}, {wait_stages, 1},
        {&currentFrame.commandBuffer, 1}, {&finishedSemaphore, 1});

    deviceContext.logDevDisp.resetFences(1, &inFlightFence);

    if (deviceContext.logDevDisp.queueSubmit(deviceContext.graphicsQueue, 1,
                                             &submitInfo,
                                             inFlightFence) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to submit Draw Command Buffer"}};
    }

    auto swapchainKRH = swapchainContext.swapchain.handle();
    auto presentInfoKHR = initializers::presentInfoKHR(
        {&finishedSemaphore, 1}, {&swapchainKRH, 1}, {&image_index, 1});

    result = deviceContext.logDevDisp.queuePresentKHR(
        deviceContext.presentQueue, &presentInfoKHR);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return recreateSwapchain(vulkan);
    } else if (result != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to present Swapchain Image"}};
    }

    vulkan.data.currentFrame =
        (vulkan.data.currentFrame + 1) % vulkan.data.frames.size();
    return {};
}

tl::expected<void, Error<EngineError>> reloadShadersAndPipeline(
    Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;

    deviceContext.logDevDisp.queueWaitIdle(deviceContext.graphicsQueue);
    return createGraphicsPipeline(vulkan);
}

void cleanup(Vulkan &vulkan) {
    cleanupSwapChain(vulkan);

    vulkan.data.frames.clear();
    vulkan.data.finishedSemaphores.clear();
    vulkan.data.commandPool = {};

    vulkan.pipelineContext.graphicsPipeline = {};
    vulkan.sceneContext.texture = Texture{};
    vulkan.sceneContext.models.clear();

    vulkan.data.descriptorAllocator.cleanup(vulkan.deviceContext);

    vulkan.pipelineContext.descriptorSetLayout = {};
    vulkan.swapchainContext.renderPass = {};

    vmaDestroyAllocator(vulkan.deviceContext.allocator);
    vkb::destroy_device(vulkan.deviceContext.logicalDevice);
    vkb::destroy_surface(vulkan.deviceContext.instance, vulkan.surface);
    vkb::destroy_instance(vulkan.deviceContext.instance);
    destroy_window_glfw(vulkan.deviceContext.window);
}

}  // namespace mpvgl::vlk
