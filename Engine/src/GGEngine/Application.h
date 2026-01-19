#pragma once

#include "Core.h"

#include "Window.h"
#include "LayerStack.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

namespace GGEngine {

    class ImGuiLayer;

    class GG_API Application 
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        void Close() { m_Running = false; }

        inline Window& GetWindow() { return *m_Window; }

        inline static Application& Get() { return *s_Instance; }

        ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        
        std::unique_ptr<Window> m_Window;
        ImGuiLayer* m_ImGuiLayer;
        bool m_Running = true;
        LayerStack m_LayerStack;

        static Application* s_Instance;
    };

    Application* CreateApplication();
}