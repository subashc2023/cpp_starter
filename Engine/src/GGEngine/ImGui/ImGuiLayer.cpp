#include "ImGuiLayer.h"

// Include glad Vulkan FIRST
#include <glad/vulkan.h>

#include "GGEngine/Application.h"
#include "Platform/Vulkan/VulkanContext.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef GG_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace GGEngine {

    // Static storage for ini filename (ImGui holds a pointer to this)
    static std::string s_IniFilename;

    static std::string GetExeDirectory()
    {
#ifdef GG_PLATFORM_WINDOWS
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            return fullPath.substr(0, lastSlash + 1);
        return "";
#else
        return "";
#endif
    }

    static std::string GetExeName()
    {
#ifdef GG_PLATFORM_WINDOWS
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of("\\/");
        size_t lastDot = fullPath.find_last_of(".");
        if (lastSlash != std::string::npos && lastDot != std::string::npos)
            return fullPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
        return "imgui";
#else
        return "imgui";
#endif
    }

    static void CheckVkResult(VkResult err)
    {
        if (err == VK_SUCCESS)
            return;
        GG_CORE_ERROR("[ImGui Vulkan] Error: VkResult = {0}", (int)err);
        if (err < 0)
            abort();
    }

    ImGuiLayer::ImGuiLayer()
        : Layer("ImGuiLayer")
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
    }

    void ImGuiLayer::OnAttach()
    {
        // Create Vulkan context
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        m_VulkanContext = new VulkanContext(window);
        m_VulkanContext->Init();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);

        // Load Vulkan functions for ImGui using GLFW's Vulkan loader
        VkInstance instance = m_VulkanContext->GetInstance();
        ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_0, [](const char* functionName, void* userData) {
            VkInstance inst = (VkInstance)userData;
            PFN_vkVoidFunction fn = glfwGetInstanceProcAddress(inst, functionName);
            return fn;
        }, (void*)instance);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = m_VulkanContext->GetInstance();
        initInfo.PhysicalDevice = m_VulkanContext->GetPhysicalDevice();
        initInfo.Device = m_VulkanContext->GetDevice();
        initInfo.QueueFamily = m_VulkanContext->GetQueueFamily();
        initInfo.Queue = m_VulkanContext->GetQueue();
        initInfo.DescriptorPool = m_VulkanContext->GetDescriptorPool();
        initInfo.MinImageCount = m_VulkanContext->GetMinImageCount();
        initInfo.ImageCount = m_VulkanContext->GetImageCount();
        initInfo.CheckVkResultFn = CheckVkResult;
        
        // Main viewport pipeline
        initInfo.PipelineInfoMain.RenderPass = m_VulkanContext->GetRenderPass();
        initInfo.PipelineInfoMain.Subpass = 0;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        
        // Secondary viewports pipeline (for multi-viewport support)
        initInfo.PipelineInfoForViewports.Subpass = 0;
        initInfo.PipelineInfoForViewports.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        
        ImGui_ImplVulkan_Init(&initInfo);

        GG_CORE_INFO("ImGui Layer attached with Vulkan backend");
    }

    void ImGuiLayer::OnDetach()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (m_VulkanContext)
        {
            delete m_VulkanContext;
            m_VulkanContext = nullptr;
        }
    }

    void ImGuiLayer::OnImGuiRender()
    {
        // Demo window for testing (can be removed later)
        static bool showDemoWindow = true;
        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);
    }

    void ImGuiLayer::OnEvent(Event& event)
    {
        if (m_BlockEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            event.m_Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
            event.m_Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }

    void ImGuiLayer::Begin()
    {
        // Handle swapchain rebuild on resize
        m_VulkanContext->BeginFrame();

        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            m_FrameStarted = false;
            return;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        m_FrameStarted = true;
    }

    void ImGuiLayer::End()
    {
        if (!m_FrameStarted)
            return;

        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::Get();
        io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

        // Rendering
        ImGui::Render();
        ImDrawData* mainDrawData = ImGui::GetDrawData();
        const bool mainIsMinimized = (mainDrawData->DisplaySize.x <= 0.0f || mainDrawData->DisplaySize.y <= 0.0f);

        ImGui_ImplVulkanH_Window* wd = m_VulkanContext->GetWindowData();
        wd->ClearValue.color.float32[0] = 0.1f;
        wd->ClearValue.color.float32[1] = 0.1f;
        wd->ClearValue.color.float32[2] = 0.1f;
        wd->ClearValue.color.float32[3] = 1.0f;

        if (!mainIsMinimized)
            FrameRender(mainDrawData);

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // Present Main Platform Window
        if (!mainIsMinimized)
            FramePresent();
    }

    // Private helper - forward to VulkanContext
    void ImGuiLayer::FrameRender(ImDrawData* drawData)
    {
        ImGui_ImplVulkanH_Window* wd = m_VulkanContext->GetWindowData();
        VkDevice device = m_VulkanContext->GetDevice();
        VkQueue queue = m_VulkanContext->GetQueue();

        VkSemaphore imageAcquiredSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkResult err = vkAcquireNextImageKHR(device, wd->Swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &wd->FrameIndex);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            m_VulkanContext->SetSwapchainRebuild(true);
        if (err == VK_ERROR_OUT_OF_DATE_KHR)
            return;

        ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
        {
            err = vkWaitForFences(device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
            err = vkResetFences(device, 1, &fd->Fence);
        }
        {
            err = vkResetCommandPool(device, fd->CommandPool, 0);
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
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

        ImGui_ImplVulkan_RenderDrawData(drawData, fd->CommandBuffer);

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
            err = vkQueueSubmit(queue, 1, &info, fd->Fence);
        }
    }

    void ImGuiLayer::FramePresent()
    {
        if (m_VulkanContext->NeedsSwapchainRebuild())
            return;

        ImGui_ImplVulkanH_Window* wd = m_VulkanContext->GetWindowData();
        VkQueue queue = m_VulkanContext->GetQueue();

        VkSemaphore renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &renderCompleteSemaphore;
        info.swapchainCount = 1;
        info.pSwapchains = &wd->Swapchain;
        info.pImageIndices = &wd->FrameIndex;
        VkResult err = vkQueuePresentKHR(queue, &info);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            m_VulkanContext->SetSwapchainRebuild(true);
        wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
    }

}
