#pragma once

// Include glad Vulkan FIRST to ensure its function pointers are used
#include <glad/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui_impl_vulkan.h"

namespace GGEngine {

    class VulkanContext
    {
    public:
        VulkanContext(GLFWwindow* windowHandle);
        ~VulkanContext();

        void Init();
        void Shutdown();

        // Frame management
        void BeginFrame();
        void EndFrame();

        // Swapchain rebuild (on resize)
        void RecreateSwapchain(int width, int height);

        // Accessors for ImGui initialization
        VkInstance GetInstance() const { return m_Instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice GetDevice() const { return m_Device; }
        VkQueue GetQueue() const { return m_Queue; }
        uint32_t GetQueueFamily() const { return m_QueueFamily; }
        VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
        VkRenderPass GetRenderPass() const { return m_WindowData.RenderPass; }
        uint32_t GetMinImageCount() const { return m_MinImageCount; }
        uint32_t GetImageCount() const { return m_WindowData.ImageCount; }

        ImGui_ImplVulkanH_Window* GetWindowData() { return &m_WindowData; }

        bool NeedsSwapchainRebuild() const { return m_SwapChainRebuild; }
        void SetSwapchainRebuild(bool rebuild) { m_SwapChainRebuild = rebuild; }

    private:
        void SetupVulkan();
        void SetupVulkanWindow(VkSurfaceKHR surface, int width, int height);
        void CleanupVulkan();
        void CleanupVulkanWindow();
        void FrameRender(ImDrawData* drawData);
        void FramePresent();

        static void CheckVkResult(VkResult err);

    private:
        GLFWwindow* m_WindowHandle = nullptr;

        VkAllocationCallbacks* m_Allocator = nullptr;
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        uint32_t m_QueueFamily = (uint32_t)-1;
        VkQueue m_Queue = VK_NULL_HANDLE;
        VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

        ImGui_ImplVulkanH_Window m_WindowData;
        uint32_t m_MinImageCount = 2;
        bool m_SwapChainRebuild = false;

#ifdef _DEBUG
        VkDebugReportCallbackEXT m_DebugReport = VK_NULL_HANDLE;
#endif
    };

}
