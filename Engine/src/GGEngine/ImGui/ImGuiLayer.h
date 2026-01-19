#pragma once

#include "GGEngine/Layer.h"

struct ImDrawData;

namespace GGEngine {

    class VulkanContext;

    class GG_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void OnAttach() override;
        void OnDetach() override;
        void OnImGuiRender() override;
        void OnEvent(Event& event) override;

        void Begin();
        void End();

        void SetBlockEvents(bool block) { m_BlockEvents = block; }
        bool IsFrameStarted() const { return m_FrameStarted; }

    private:
        void FrameRender(ImDrawData* drawData);
        void FramePresent();

    private:
        bool m_BlockEvents = true;
        bool m_FrameStarted = false;
        float m_Time = 0.0f;
        VulkanContext* m_VulkanContext = nullptr;
    };

}
