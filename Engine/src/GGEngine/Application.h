#pragma once

#include "Core.h"
#include "Window.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

namespace GGEngine {

    class GG_API Application 
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);
    private:
        bool OnWindowClose(WindowCloseEvent& e);
        
        std::unique_ptr<Window> m_Window;
        bool m_Running = true;
    };

    Application* CreateApplication();
}