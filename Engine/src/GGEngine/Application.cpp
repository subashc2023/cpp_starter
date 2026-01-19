#include "Application.h"

#include "GGEngine/Window.h"
#include "GGEngine/Events/ApplicationEvent.h"
#include "GGEngine/Log.h"

namespace GGEngine {
    Application::Application() 
    {
        m_Window = std::unique_ptr<Window>(Window::Create());
    }
    Application::~Application() 
    {

    }

    void Application::Run() 
    {
        while (m_Running) 
        {
            m_Window->OnUpdate();
        }
    }
}