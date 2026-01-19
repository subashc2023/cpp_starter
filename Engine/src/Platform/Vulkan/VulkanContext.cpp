#include "VulkanContext.h"
#include "GGEngine/Log.h"

#include <glad/vulkan.h>
#include <stdio.h>
#include <stdlib.h>

namespace GGEngine {

#ifdef _DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReport(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* pUserData)
    {
        (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix;
        GG_CORE_ERROR("[Vulkan] Debug report from ObjectType: {0}\nMessage: {1}", objectType, pMessage);
        return VK_FALSE;
    }
#endif

    static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
    {
        for (const VkExtensionProperties& p : properties)
            if (strcmp(p.extensionName, extension) == 0)
                return true;
        return false;
    }

    void VulkanContext::CheckVkResult(VkResult err)
    {
        if (err == VK_SUCCESS)
            return;
        GG_CORE_ERROR("[Vulkan] Error: VkResult = {0}", (int)err);
        if (err < 0)
            abort();
    }

    VulkanContext::VulkanContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
    }

    VulkanContext::~VulkanContext()
    {
        Shutdown();
    }

    void VulkanContext::Init()
    {
        if (!glfwVulkanSupported())
        {
            GG_CORE_CRITICAL("GLFW: Vulkan Not Supported!");
            return;
        }

        ImVector<const char*> extensions;
        uint32_t extensionsCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
        for (uint32_t i = 0; i < extensionsCount; i++)
            extensions.push_back(glfwExtensions[i]);

        SetupVulkan();

        // Create Window Surface
        VkSurfaceKHR surface;
        VkResult err = glfwCreateWindowSurface(m_Instance, m_WindowHandle, m_Allocator, &surface);
        CheckVkResult(err);

        // Create Framebuffers
        int w, h;
        glfwGetFramebufferSize(m_WindowHandle, &w, &h);
        SetupVulkanWindow(surface, w, h);

        GG_CORE_INFO("Vulkan Context initialized successfully");
    }

    void VulkanContext::Shutdown()
    {
        VkResult err = vkDeviceWaitIdle(m_Device);
        CheckVkResult(err);

        CleanupVulkanWindow();
        CleanupVulkan();
    }

    void VulkanContext::SetupVulkan()
    {
        VkResult err;

        // Initialize glad Vulkan loader (first call to load instance creation functions)
        int gladVersion = gladLoaderLoadVulkan(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
        if (!gladVersion) {
            GG_CORE_CRITICAL("Failed to initialize glad Vulkan loader!");
            return;
        }
        GG_CORE_INFO("Glad Vulkan loader initialized");

        // Create Vulkan Instance
        {
            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

            // Enumerate available extensions
            uint32_t propertiesCount;
            ImVector<VkExtensionProperties> properties;
            vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, nullptr);
            properties.resize(propertiesCount);
            err = vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, properties.Data);
            CheckVkResult(err);

            ImVector<const char*> instanceExtensions;
            uint32_t glfwExtCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
            for (uint32_t i = 0; i < glfwExtCount; i++)
                instanceExtensions.push_back(glfwExtensions[i]);

            // Enable required extensions
            if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
                instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
            if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
            {
                instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
                createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            }
#endif

            // Enabling validation layers
#ifdef _DEBUG
            const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
            createInfo.enabledLayerCount = 1;
            createInfo.ppEnabledLayerNames = layers;
            instanceExtensions.push_back("VK_EXT_debug_report");
#endif

            createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.Size;
            createInfo.ppEnabledExtensionNames = instanceExtensions.Data;
            err = vkCreateInstance(&createInfo, m_Allocator, &m_Instance);
            CheckVkResult(err);

            // Load instance-level Vulkan functions
            gladLoaderLoadVulkan(m_Instance, VK_NULL_HANDLE, VK_NULL_HANDLE);

            // Setup the debug report callback
#ifdef _DEBUG
            auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugReportCallbackEXT");
            IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
            VkDebugReportCallbackCreateInfoEXT debugReportCI = {};
            debugReportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debugReportCI.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debugReportCI.pfnCallback = VulkanDebugReport;
            debugReportCI.pUserData = nullptr;
            err = f_vkCreateDebugReportCallbackEXT(m_Instance, &debugReportCI, m_Allocator, &m_DebugReport);
            CheckVkResult(err);
#endif
        }

        // Select Physical Device (GPU)
        m_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(m_Instance);
        IM_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE);

        // Load physical device-level functions
        gladLoaderLoadVulkan(m_Instance, m_PhysicalDevice, VK_NULL_HANDLE);

        // Select graphics queue family
        m_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(m_PhysicalDevice);
        IM_ASSERT(m_QueueFamily != (uint32_t)-1);

        // Create Logical Device (with 1 queue)
        {
            ImVector<const char*> deviceExtensions;
            deviceExtensions.push_back("VK_KHR_swapchain");

            // Enumerate physical device extension
            uint32_t propertiesCount;
            ImVector<VkExtensionProperties> properties;
            vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &propertiesCount, nullptr);
            properties.resize(propertiesCount);
            vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &propertiesCount, properties.Data);

#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
            if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
                deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

            const float queuePriority[] = { 1.0f };
            VkDeviceQueueCreateInfo queueInfo[1] = {};
            queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo[0].queueFamilyIndex = m_QueueFamily;
            queueInfo[0].queueCount = 1;
            queueInfo[0].pQueuePriorities = queuePriority;

            VkDeviceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = sizeof(queueInfo) / sizeof(queueInfo[0]);
            createInfo.pQueueCreateInfos = queueInfo;
            createInfo.enabledExtensionCount = (uint32_t)deviceExtensions.Size;
            createInfo.ppEnabledExtensionNames = deviceExtensions.Data;
            err = vkCreateDevice(m_PhysicalDevice, &createInfo, m_Allocator, &m_Device);
            CheckVkResult(err);

            // Load device-level Vulkan functions
            gladLoaderLoadVulkan(m_Instance, m_PhysicalDevice, m_Device);

            vkGetDeviceQueue(m_Device, m_QueueFamily, 0, &m_Queue);
        }

        // Create Descriptor Pool
        {
            VkDescriptorPoolSize poolSizes[] =
            {
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
            };
            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.maxSets = 0;
            for (VkDescriptorPoolSize& poolSize : poolSizes)
                poolInfo.maxSets += poolSize.descriptorCount;
            poolInfo.poolSizeCount = (uint32_t)IM_COUNTOF(poolSizes);
            poolInfo.pPoolSizes = poolSizes;
            err = vkCreateDescriptorPool(m_Device, &poolInfo, m_Allocator, &m_DescriptorPool);
            CheckVkResult(err);
        }
    }

    void VulkanContext::SetupVulkanWindow(VkSurfaceKHR surface, int width, int height)
    {
        ImGui_ImplVulkanH_Window* wd = &m_WindowData;

        // Check for WSI support
        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_QueueFamily, surface, &res);
        if (res != VK_TRUE)
        {
            GG_CORE_CRITICAL("Error: no WSI support on physical device 0");
            return;
        }

        // Select Surface Format
        const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
        const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        wd->Surface = surface;
        wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(m_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_COUNTOF(requestSurfaceImageFormat), requestSurfaceColorSpace);

        // Select Present Mode (VSync)
        VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
        wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(m_PhysicalDevice, wd->Surface, &presentModes[0], IM_COUNTOF(presentModes));

        // Create SwapChain, RenderPass, Framebuffer, etc.
        IM_ASSERT(m_MinImageCount >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(m_Instance, m_PhysicalDevice, m_Device, wd, m_QueueFamily, m_Allocator, width, height, m_MinImageCount, 0);
    }

    void VulkanContext::CleanupVulkan()
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, m_Allocator);

#ifdef _DEBUG
        auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugReportCallbackEXT");
        f_vkDestroyDebugReportCallbackEXT(m_Instance, m_DebugReport, m_Allocator);
#endif

        vkDestroyDevice(m_Device, m_Allocator);
        vkDestroyInstance(m_Instance, m_Allocator);
    }

    void VulkanContext::CleanupVulkanWindow()
    {
        ImGui_ImplVulkanH_DestroyWindow(m_Instance, m_Device, &m_WindowData, m_Allocator);
        vkDestroySurfaceKHR(m_Instance, m_WindowData.Surface, m_Allocator);
    }

    void VulkanContext::RecreateSwapchain(int width, int height)
    {
        ImGui_ImplVulkan_SetMinImageCount(m_MinImageCount);
        ImGui_ImplVulkanH_CreateOrResizeWindow(m_Instance, m_PhysicalDevice, m_Device, &m_WindowData, m_QueueFamily, m_Allocator, width, height, m_MinImageCount, 0);
        m_WindowData.FrameIndex = 0;
        m_SwapChainRebuild = false;
    }

    void VulkanContext::BeginFrame()
    {
        // Check if we need to resize
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(m_WindowHandle, &fbWidth, &fbHeight);
        if (fbWidth > 0 && fbHeight > 0 && (m_SwapChainRebuild || m_WindowData.Width != fbWidth || m_WindowData.Height != fbHeight))
        {
            RecreateSwapchain(fbWidth, fbHeight);
        }
    }

    void VulkanContext::EndFrame()
    {
        // This is called after ImGui::Render() with the draw data
    }

    void VulkanContext::FrameRender(ImDrawData* drawData)
    {
        ImGui_ImplVulkanH_Window* wd = &m_WindowData;

        VkSemaphore imageAcquiredSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkResult err = vkAcquireNextImageKHR(m_Device, wd->Swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &wd->FrameIndex);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            m_SwapChainRebuild = true;
        if (err == VK_ERROR_OUT_OF_DATE_KHR)
            return;
        if (err != VK_SUBOPTIMAL_KHR)
            CheckVkResult(err);

        ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
        {
            err = vkWaitForFences(m_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
            CheckVkResult(err);

            err = vkResetFences(m_Device, 1, &fd->Fence);
            CheckVkResult(err);
        }
        {
            err = vkResetCommandPool(m_Device, fd->CommandPool, 0);
            CheckVkResult(err);
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
            CheckVkResult(err);
        }
        {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = wd->RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width = wd->Width;
            info.renderArea.extent.height = wd->Height;
            info.clearValueCount = 1;
            info.pClearValues = &wd->ClearValue;
            vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(drawData, fd->CommandBuffer);

        // Submit command buffer
        vkCmdEndRenderPass(fd->CommandBuffer);
        {
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &imageAcquiredSemaphore;
            info.pWaitDstStageMask = &waitStage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &fd->CommandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &renderCompleteSemaphore;

            err = vkEndCommandBuffer(fd->CommandBuffer);
            CheckVkResult(err);
            err = vkQueueSubmit(m_Queue, 1, &info, fd->Fence);
            CheckVkResult(err);
        }
    }

    void VulkanContext::FramePresent()
    {
        if (m_SwapChainRebuild)
            return;

        ImGui_ImplVulkanH_Window* wd = &m_WindowData;
        VkSemaphore renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &renderCompleteSemaphore;
        info.swapchainCount = 1;
        info.pSwapchains = &wd->Swapchain;
        info.pImageIndices = &wd->FrameIndex;
        VkResult err = vkQueuePresentKHR(m_Queue, &info);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            m_SwapChainRebuild = true;
        if (err == VK_ERROR_OUT_OF_DATE_KHR)
            return;
        if (err != VK_SUBOPTIMAL_KHR)
            CheckVkResult(err);
        wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
    }

}
