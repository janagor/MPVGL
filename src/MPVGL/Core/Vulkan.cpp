#define GLM_FORCE_RADIANS
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <stb/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/CommandPool.hpp"
#include "MPVGL/Core/Vulkan/Descriptor.hpp"
#include "MPVGL/Core/Vulkan/GraphicsPipeline.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/InstanceBuilder.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"
#include "MPVGL/Core/Vulkan/LogicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/Model.hpp"
#include "MPVGL/Core/Vulkan/PhysicalDeviceBuilder.hpp"
#include "MPVGL/Core/Vulkan/RenderPass.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Core/Vulkan/SyncObjects.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

#include "config.hpp"

namespace {

template <typename T>
auto vkb_to_expected(vkb::Result<T> &&res, std::string const &msg)
    -> tl::expected<T, mpvgl::Error<mpvgl::EngineError>> {
    if (!res) {
        return tl::unexpected{
            mpvgl::Error<mpvgl::EngineError>{std::move(res).error(), msg}};
    }
    return std::move(res).value();
}

}  // namespace

namespace mpvgl::vlk {

constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr u32 DESCRIPTOR_POOL_MAX_SETS = 1000;

auto deviceInitialization(Vulkan &vulkan, int width, int height,
                          std::string const &title, GLFWmonitor *monitor,
                          GLFWwindow *share, bool resize)
    -> tl::expected<void, Error<EngineError>> {
    auto &surface = vulkan.surface;
    auto &deviceContext = vulkan.deviceContext;

    return createWindow(width, height, title, monitor, share, resize)
        .transform_error(
            [](auto error) -> auto { return Error{std::move(error)}; })
        .and_then([&](auto window) -> auto {
            deviceContext.window = window;

            return InstanceBuilder::getInstance().transform_error(
                [](auto error) -> auto {
                    return Error<EngineError>{error,
                                              "Failed to create Instance"};
                });
        })
        .and_then([&](auto instance) -> auto {
            deviceContext.instance = instance;
            deviceContext.instDisp = deviceContext.instance.make_table();
            surface = create_surface_glfw(deviceContext.instance,
                                          deviceContext.window, nullptr);

            return PhysicalDeviceBuilder::getPhysicalDevice(
                       deviceContext.instance, surface)
                .transform_error([](auto error) -> auto {
                    return Error<EngineError>{
                        error, "Failed to create Physical Device"};
                });
        })
        .and_then([](auto physicalDevice) -> auto {
            return LogicalDeviceBuilder::getLogicalDevice(physicalDevice)
                .transform_error([](auto error) -> auto {
                    return Error<EngineError>{
                        error, "Failed to create Logical Device"};
                });
        })
        .and_then([&](auto logicalDevice)
                      -> tl::expected<void, Error<EngineError>> {
            deviceContext.logicalDevice = std::move(logicalDevice);
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

auto createSwapchain(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>> {
    return Swapchain::create(vulkan.deviceContext)
        .transform([&vulkan](Swapchain swapchain) -> void {
            vulkan.swapchainContext.swapchain = std::move(swapchain);
        });
}

auto getQueues(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>> {
    auto &logicalDevice = vulkan.deviceContext.logicalDevice;
    auto &deviceContext = vulkan.deviceContext;

    return vkb_to_expected(logicalDevice.get_queue(vkb::QueueType::graphics),
                           "Failed to get Graphics Queue")
        .and_then(
            [&deviceContext, &logicalDevice](VkQueue gQueue)
                -> tl::expected<VkQueue_T *, mpvgl::Error<mpvgl::EngineError>> {
                deviceContext.graphicsQueue = gQueue;
                return vkb_to_expected(
                    logicalDevice.get_queue(vkb::QueueType::present),
                    "Failed to get Present Queue");
            })
        .transform([&](VkQueue pQueue) -> void {
            deviceContext.presentQueue = pQueue;
        });
}

auto bootstrap(Vulkan &vulkan, int width, int height, std::string const &title,
               GLFWmonitor *monitor, GLFWwindow *share, bool resize)
    -> tl::expected<void, Error<EngineError>> {
    return deviceInitialization(vulkan, width, height, title, monitor, share,
                                resize)
        .and_then([&] -> tl::expected<void, Error<EngineError>> {
            return createSwapchain(vulkan);
        })
        .and_then([&] -> tl::expected<void, Error<EngineError>> {
            return getQueues(vulkan);
        });
}

auto setupRenderingPipeline(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    return createRenderPass(vulkan)
        .and_then([&] -> tl::expected<void, Error<EngineError>> {
            return createDescriptorSetLayout(vulkan);
        })
        .and_then([&] -> tl::expected<void, Error<EngineError>> {
            return createGraphicsPipeline(vulkan);
        });
}

auto setupRenderTargets(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    return createDepthResources(vulkan).and_then(
        [&] -> tl::expected<void, Error<EngineError>> {
            return createFramebuffers(vulkan);
        });
}

auto loadAndPrepareAssets(Vulkan &vulkan, Scene const &scene)
    -> tl::expected<void, Error<EngineError>> {
    return vlk::createUniformBuffers(vulkan).and_then(
        [&] -> tl::expected<void, Error<EngineError>> {
            return vlk::loadScene(vulkan, scene);
        });
}

auto setupDescriptorsAndSync(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    return vlk::createDescriptorPool(vulkan)
        .and_then([&] -> tl::expected<void, Error<EngineError>> {
            return vlk::createCommandPool(vulkan);
        })
        .and_then([&] -> tl::expected<void, Error<EngineError>> {
            return vlk::createCommandBuffers(vulkan);
        })
        .and_then([&] -> tl::expected<void, Error<EngineError>> {
            return vlk::createSyncObjects(vulkan);
        });
}

auto createRenderPass(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    RenderPassBuilder builder{};

    VkAttachmentDescription const colorAttachment = {
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
        .transform([&](RenderPass renderPass) -> void {
            vulkan.swapchainContext.renderPass = std::move(renderPass);
        });
}

auto createDescriptorSetLayout(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    DescriptorLayoutBuilder builder{};

    builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                       VK_SHADER_STAGE_VERTEX_BIT);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_FRAGMENT_BIT);

    return builder.build(vulkan.deviceContext)
        .transform([&](DescriptorSetLayout layout) -> void {
            vulkan.pipelineContext.descriptorSetLayout = std::move(layout);
        });
}

auto createGraphicsPipeline(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    auto &deviceContext = vulkan.deviceContext;
    auto &pipelineContext = vulkan.pipelineContext;
    auto &swapchainContext = vulkan.swapchainContext;

    return GraphicsPipeline::create(
               deviceContext, swapchainContext.renderPass.handle(),
               pipelineContext.descriptorSetLayout.handle(),
               std::string(SOURCE_DIRECTORY) + "/shaders/triangle.vert.spv",
               std::string(SOURCE_DIRECTORY) + "/shaders/triangle.frag.spv")
        .transform([&](GraphicsPipeline pipeline) -> void {
            pipelineContext.graphicsPipeline = std::move(pipeline);
        });
}

auto createFramebuffers(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
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

auto createCommandPool(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    auto &deviceContext = vulkan.deviceContext;
    return vkb_to_expected(deviceContext.logicalDevice.get_queue_index(
                               vkb::QueueType::graphics),
                           "Failed to get Queue Index")
        .and_then(
            [&](u32 queueIndex) -> tl::expected<void, Error<EngineError>> {
                return CommandPool::create(deviceContext, queueIndex)
                    .transform([&vulkan](CommandPool pool) -> void {
                        vulkan.data.commandPool = std::move(pool);
                    });
            });
}

auto createDepthResources(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    auto &swapchainContext = vulkan.swapchainContext;
    auto &deviceContext = vulkan.deviceContext;

    return findDepthFormat(vulkan).and_then(
        [&](VkFormat format) -> tl::expected<void, Error<EngineError>> {
            return Texture::createDepthTexture(
                       deviceContext, swapchainContext.swapchain.extent().width,
                       swapchainContext.swapchain.extent().height, format)
                .transform([&](Texture depthTex) -> void {
                    swapchainContext.depthTexture = std::move(depthTex);
                });
        });
}

auto loadScene(Vulkan &vulkan, Scene const &scene)
    -> tl::expected<void, Error<EngineError>> {
    auto &sceneContext = vulkan.sceneContext;
    auto &pipelineContext = vulkan.pipelineContext;
    auto &deviceContext = vulkan.deviceContext;

    for (auto const &node : scene.nodes()) {
        auto modelRes = Model::create(
            deviceContext, vulkan.data.commandPool.handle(),
            deviceContext.graphicsQueue, node.mesh.vertices, node.mesh.indices);
        if (!modelRes) {
            return tl::unexpected{modelRes.error()};
        }
        sceneContext.models.push_back(std::move(modelRes.value()));

        if (!sceneContext.materials.contains(node.texturePath)) {
            MaterialData newMaterial{};

            auto texRes = Texture::loadFromFile(
                deviceContext, vulkan.data.commandPool.handle(),
                deviceContext.graphicsQueue, node.texturePath);
            if (!texRes) {
                return tl::unexpected{texRes.error()};
            }
            newMaterial.texture = std::move(texRes.value());

            size_t const framesInFlight = vulkan.data.frames.size();
            newMaterial.descriptorSets.resize(framesInFlight);

            for (size_t i = 0; i < framesInFlight; i++) {
                auto setRes = vulkan.data.descriptorAllocator.allocate(
                    deviceContext,
                    pipelineContext.descriptorSetLayout.handle());

                if (!setRes) {
                    return tl::unexpected{setRes.error()};
                }
                newMaterial.descriptorSets[i] = setRes.value();

                DescriptorWriter writer{};
                writer
                    .writeBuffer(
                        0, vulkan.data.frames.at(i).uniformBuffer.handle(),
                        sizeof(UniformBufferObject), 0,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    .writeImage(1, newMaterial.texture.imageView(),
                                newMaterial.texture.sampler(),
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .updateSet(deviceContext, newMaterial.descriptorSets[i]);
            }

            sceneContext.materials[node.texturePath] = std::move(newMaterial);
        }

        RenderObject obj{};
        obj.model = &sceneContext.models.back();
        obj.material = &sceneContext.materials[node.texturePath];

        obj.transformMatrix = node.transform;

        sceneContext.renderables.push_back(obj);
    }

    return {};
}

auto createUniformBuffers(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    auto bufferSize = sizeof(UniformBufferObject);

    for (size_t i = {}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto result =
            Buffer::create(
                vulkan.deviceContext, bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .and_then([&](Buffer buffer)
                              -> tl::expected<void, Error<EngineError>> {
                    return buffer.map().transform(
                        [&](void *mappedData) -> void {
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

auto createDescriptorPool(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    std::vector<DescriptorAllocator::PoolSizeRatio> ratios = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0F},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0F}};

    return vulkan.data.descriptorAllocator.init(
        vulkan.deviceContext, DESCRIPTOR_POOL_MAX_SETS, ratios);
}

auto createCommandBuffers(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    vulkan.data.frames.resize(MAX_FRAMES_IN_FLIGHT);
    auto framesCount = vulkan.data.frames.size();
    std::vector<VkCommandBuffer> allocatedBuffers(framesCount);

    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.commandPool.handle(), VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<u32>(framesCount));

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

auto createSyncObjects(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
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

    for (auto &frame : vulkan.data.frames) {
        auto semRes = Semaphore::create(deviceContext);
        if (!semRes) {
            return tl::unexpected{semRes.error()};
        }
        frame.availableSemaphore = std::move(semRes.value());

        auto fenceRes =
            Fence::create(deviceContext, VK_FENCE_CREATE_SIGNALED_BIT);
        if (!fenceRes) {
            return tl::unexpected{fenceRes.error()};
        }
        frame.inFlightFence = std::move(fenceRes.value());
    }
    return {};
}

auto drawFrame(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>> {
    auto &deviceContext = vulkan.deviceContext;
    auto &swapchainContext = vulkan.swapchainContext;

    auto &currentFrame = vulkan.data.frames[vulkan.data.currentFrame];

    VkFence inFlightFence = currentFrame.inFlightFence.handle();
    VkSemaphore availableSemaphore = currentFrame.availableSemaphore.handle();

    deviceContext.logDevDisp.waitForFences(1, &inFlightFence, VK_TRUE,
                                           std::numeric_limits<u64>::max());

    u32 image_index = 0;
    VkResult result = deviceContext.logDevDisp.acquireNextImageKHR(
        swapchainContext.swapchain.handle(), std::numeric_limits<u64>::max(),
        availableSemaphore, VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreateSwapchain(vulkan);
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to acquire Swapchain Image"}};
    }

    if (vulkan.data.imageInFlight.at(image_index) != VK_NULL_HANDLE) {
        deviceContext.logDevDisp.waitForFences(
            1, &vulkan.data.imageInFlight.at(image_index), VK_TRUE,
            std::numeric_limits<u64>::max());
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

    auto wait_stages = std::array<VkPipelineStageFlags, 1>{
        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}};

    auto submitInfo = initializers::submitInfo(
        {&availableSemaphore, 1}, {wait_stages.data(), 1},
        {&currentFrame.commandBuffer, 1}, {&finishedSemaphore, 1});

    deviceContext.logDevDisp.resetFences(1, &inFlightFence);

    if (deviceContext.logDevDisp.queueSubmit(deviceContext.graphicsQueue, 1,
                                             &submitInfo,
                                             inFlightFence) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to submit Draw Command Buffer"}};
    }

    auto *swapchainKRH = swapchainContext.swapchain.handle();
    auto presentInfoKHR = initializers::presentInfoKHR(
        {&finishedSemaphore, 1}, {&swapchainKRH, 1}, {&image_index, 1});

    result = deviceContext.logDevDisp.queuePresentKHR(
        deviceContext.presentQueue, &presentInfoKHR);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return recreateSwapchain(vulkan);
    }
    if (result != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to present Swapchain Image"}};
    }

    vulkan.data.currentFrame =
        (vulkan.data.currentFrame + 1) % vulkan.data.frames.size();
    return {};
}

auto reloadShadersAndPipeline(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
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
    vulkan.sceneContext.models.clear();
    vulkan.sceneContext.materials.clear();

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
