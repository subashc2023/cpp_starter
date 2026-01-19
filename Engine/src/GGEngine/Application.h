#pragma once

#include "Core.h"
#include "Events/Event.h"

namespace GGEngine {

    class GG_API Application 
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    };

    Application* CreateApplication();
}