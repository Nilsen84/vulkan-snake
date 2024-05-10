#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_float3.hpp>

class Renderer {
public:
    explicit Renderer(GLFWwindow* window);
    ~Renderer();

    void begin(glm::vec4 clearColor);
    void setColor(glm::vec3 color);
    void drawQuad(const glm::mat4& transform);
    void end();

private:
    void createInstance(std::span<const char*> extensions);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain(GLFWwindow *window);
    void createImageViews();
    void createGraphicsPipeline();
    void createCommandPool();

private:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    static constexpr auto IMAGE_FORMAT = vk::Format::eB8G8R8A8Unorm;
    static constexpr auto COLOR_SPACE = vk::ColorSpaceKHR::eSrgbNonlinear;

    struct FrameData {
        vk::UniqueSemaphore imageAcquired, renderFinished;
        vk::UniqueFence fence;
    };

    vk::DynamicLoader m_dynamicLoader {};

    vk::UniqueInstance m_instance;
    vk::UniqueSurfaceKHR m_surface;
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;
    uint32_t m_queueFamily{};
    vk::Queue m_queue;
    vk::UniqueSwapchainKHR m_swapchain;
    vk::Extent2D m_swapchainExtent;
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_imageViews;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_pipeline;

    vk::UniqueCommandPool m_commandPool;
    std::vector<vk::UniqueCommandBuffer> m_commandBuffers;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_frames;
    uint32_t m_currentFrame = 0;
    uint32_t m_currentImage = 0;
};
