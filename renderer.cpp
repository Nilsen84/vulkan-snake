#include "renderer.h"

#include <iostream>
#include <ranges>
#include <array>
#include <functional>

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include "basic.vert.h"
#include "basic.frag.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define VK_RESULT_CHECK(x, msg) vk::resultCheck(static_cast<vk::Result>(x), msg)

Renderer::Renderer(GLFWwindow *window) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_dynamicLoader);
    uint32_t count;
    VkSurfaceKHR surface;

    createInstance({glfwGetRequiredInstanceExtensions(&count), count});
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

    VK_RESULT_CHECK(glfwCreateWindowSurface(*m_instance, window, nullptr, &surface), "glfwCreateWindowSurface");
    m_surface = vk::UniqueSurfaceKHR(surface, *m_instance);

    pickPhysicalDevice();
    std::cout << m_physicalDevice.getProperties().deviceName << std::endl;
    createLogicalDevice();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
    createSwapChain(window);
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
}

Renderer::~Renderer() {
    m_device->waitIdle();
}

void Renderer::begin(glm::vec4 clearColor) {
    auto& cmdBuf = m_commandBuffers[m_currentFrame];
    auto& frame = m_frames[m_currentFrame];

    std::ignore = m_device->waitForFences({ *frame.fence }, false, UINT64_MAX);
    m_device->resetFences({ *frame.fence });

    m_currentImage = m_device->acquireNextImageKHR(*m_swapchain, UINT64_MAX, *frame.imageAcquired, {}).value;

    auto colorAttachment = vk::RenderingAttachmentInfo{ *m_imageViews[m_currentImage] }
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setClearValue({ vk::ClearColorValue{ clearColor.r, clearColor.g, clearColor.b, clearColor.a } })
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore);

    vk::ImageMemoryBarrier barrier {
            {},
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            m_images[m_currentImage],
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    };

    cmdBuf->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    cmdBuf->pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        0, nullptr, 0, nullptr,
        1, &barrier
    );
    cmdBuf->beginRendering({ {}, { { 0, 0 }, m_swapchainExtent }, 1, {}, 1, &colorAttachment });
    cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
}

void Renderer::setColor(glm::vec3 color) {
    m_commandBuffers[m_currentFrame]->pushConstants(*m_pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), sizeof(color), value_ptr(color));
}

void Renderer::drawQuad(const glm::mat4 &transform) {
    auto& cmdBuf = m_commandBuffers[m_currentFrame];
    cmdBuf->pushConstants(*m_pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), value_ptr(transform));
    cmdBuf->draw(6, 1, 0, 0);
}

void Renderer::end() {
    auto& frame = m_frames[m_currentFrame];
    auto& cmdBuf = m_commandBuffers[m_currentFrame];

    vk::ImageMemoryBarrier barrier {
        vk::AccessFlagBits::eColorAttachmentWrite,
        {},
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        m_images[m_currentImage],
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    };

    cmdBuf->endRendering();
    cmdBuf->pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {},
        0, nullptr, 0, nullptr,
        1, &barrier
    );
    cmdBuf->end();

    constexpr vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    m_queue.submit({ vk::SubmitInfo{}
        .setCommandBuffers({ *cmdBuf })
        .setWaitDstStageMask({ dstStageMask })
        .setWaitSemaphores({ *frame.imageAcquired })
        .setSignalSemaphores({ *frame.renderFinished })}, *frame.fence);

    std::ignore = m_queue.presentKHR(vk::PresentInfoKHR{}
        .setWaitSemaphores({ *frame.renderFinished })
        .setSwapchains( { *m_swapchain } )
        .setPImageIndices(&m_currentImage));

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::createInstance(std::span<const char *> extensions) {
    vk::ApplicationInfo appInfo(nullptr, 1u, nullptr, 1u, VK_API_VERSION_1_3);

    std::array layers{
#ifndef NDEBUG
        "VK_LAYER_KHRONOS_validation",
#endif
        "",
    };

    m_instance = vk::createInstanceUnique({
        {},
        &appInfo,
        layers.size() - 1, layers.data(),
        (uint32_t) extensions.size(), extensions.data()
    });
}

void Renderer::pickPhysicalDevice() {
    auto devices = m_instance->enumeratePhysicalDevices();
    if (devices.empty()) throw std::runtime_error("failed to find GPU with Vulkan support");

    for (auto device : devices) {
        if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_physicalDevice = device;
            return;
        }
    }

    m_physicalDevice = devices.front();
}

void Renderer::createLogicalDevice() {
    auto families = m_physicalDevice.getQueueFamilyProperties();
    for (m_queueFamily = 0; m_queueFamily < families.size(); m_queueFamily++) {
        if (families[m_queueFamily].queueFlags & vk::QueueFlagBits::eGraphics && m_physicalDevice.getSurfaceSupportKHR(m_queueFamily, *m_surface)) {
            goto found;
        }
    }
    throw std::runtime_error("failed to find appropriate queue family");

found:
    constexpr float priority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo({}, m_queueFamily, 1, &priority);

    std::array extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRendering(true);

    m_device = m_physicalDevice.createDeviceUnique({
        {},
        1, &queueInfo,
        0, nullptr,
        extensions.size(), extensions.data(),
        {}, &dynamicRendering
    });
    m_queue = m_device->getQueue(m_queueFamily, 0);
}

void Renderer::createSwapChain(GLFWwindow *window) {
    auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
    m_swapchainExtent = capabilities.currentExtent;

    if (m_swapchainExtent == vk::Extent2D{ UINT32_MAX, UINT32_MAX }) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        m_swapchainExtent.width = glm::clamp((uint32_t)width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        m_swapchainExtent.height = glm::clamp((uint32_t)height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    m_swapchain = m_device->createSwapchainKHRUnique({
        {},
        *m_surface,
        2,
        IMAGE_FORMAT,
        COLOR_SPACE,
        m_swapchainExtent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::PresentModeKHR::eFifo,
        true,
        {}
    });

    m_images = m_device->getSwapchainImagesKHR(*m_swapchain);
}

void Renderer::createImageViews() {
    std::ranges::transform(m_images, std::back_inserter(m_imageViews), [this](vk::Image image) {
        return m_device->createImageViewUnique({
            {},
            image,
            vk::ImageViewType::e2D,
            IMAGE_FORMAT,
            {},
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        });
    });
}

void Renderer::createCommandPool() {
    m_commandPool = m_device->createCommandPoolUnique({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_queueFamily });

    m_commandBuffers = m_device->allocateCommandBuffersUnique({
        *m_commandPool,
        vk::CommandBufferLevel::ePrimary,
        MAX_FRAMES_IN_FLIGHT
    });

    for (auto& frame : m_frames) {
        frame.imageAcquired = m_device->createSemaphoreUnique({});
        frame.renderFinished = m_device->createSemaphoreUnique({});
        frame.fence = m_device->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });
    }
}

void Renderer::createGraphicsPipeline() {
    vk::UniqueShaderModule vertex = m_device->createShaderModuleUnique({ {}, sizeof(basic_vert), basic_vert });
    vk::UniqueShaderModule fragment = m_device->createShaderModuleUnique({ {}, sizeof(basic_frag), basic_frag });

    std::array stages {
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertex, "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragment, "main"),
    };

    vk::PipelineVertexInputStateCreateInfo vertextInputInfo({}, 0, nullptr, 0, nullptr);
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);

    vk::Viewport viewport { 0, 0, (float)m_swapchainExtent.width, (float)m_swapchainExtent.height, 0.0f, 1.0f};
    vk::Rect2D scissor { { 0, 0 }, m_swapchainExtent };

    vk::PipelineViewportStateCreateInfo viewportState({}, { viewport }, { scissor });
    vk::PipelineRasterizationStateCreateInfo rasterizer {
        {},
        false,
        false,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eClockwise,
        false, {}, {}, {},
        1.0f
    };
    vk::PipelineMultisampleStateCreateInfo multisampling {
        {},
        vk::SampleCountFlagBits::e1,
        false,
        1.0f,
        nullptr,
        false,
        false
    };
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        false,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    vk::PipelineColorBlendStateCreateInfo colorBlending {
        {},
        false,
        vk::LogicOp::eCopy,
        { colorBlendAttachment }
    };

    vk::PipelineRenderingCreateInfo pipelineRenderingInfo {
        {},
        1,
        &IMAGE_FORMAT
    };

    std::array pushConstants {
        vk::PushConstantRange {
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(glm::mat4)
        },
        vk::PushConstantRange {
            vk::ShaderStageFlagBits::eFragment,
            sizeof(glm::mat4),
            sizeof(glm::vec3)
        }
    };

    m_pipelineLayout = m_device->createPipelineLayoutUnique({ {},{}, pushConstants });
    std::tie(std::ignore, m_pipeline) = m_device->createGraphicsPipelineUnique({}, {
        {},
        stages,
        &vertextInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr,
        &colorBlending,
        nullptr,
        *m_pipelineLayout,
        {},
        0,
        {},
        0,
        &pipelineRenderingInfo
    }).asTuple();
}
