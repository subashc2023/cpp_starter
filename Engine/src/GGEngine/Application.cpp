#include "Application.h"

#include "GGEngine/Events/ApplicationEvent.h"
#include "GGEngine/Log.h"

namespace GGEngine {
    Application::Application() {
    }
    Application::~Application() {
    }

    void Application::Run() {

        WindowResizeEvent e(1280, 720);
        if (e.IsInCategory(EventCategoryApplication)) 
        {
            GG_TRACE(e);
        }
        if (e.IsInCategory(EventCategoryInput)) {
            GG_TRACE(e);
        }


        while (true);
    }
}