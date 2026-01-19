#pragma once

#ifdef GG_PLATFORM_WINDOWS

extern GGEngine::Application* GGEngine::CreateApplication();

int main(int argc, char** argv) 
{
    GGEngine::Log::Init();
    GG_CORE_TRACE("Initialized Log!");
    GG_CORE_INFO("Initialized Log!");
    GG_CORE_WARN("Initialized Log!");
    GG_CORE_ERROR("Initialized Log!");
    GG_CORE_CRITICAL("Initialized Log!");

    GG_TRACE("Initialized Log!");
    GG_INFO("Initialized Log!");
    GG_WARN("Initialized Log!");
    GG_ERROR("Initialized Log!");
    GG_CRITICAL("Initialized Log!");

    int a = 5;
    GG_INFO("a is {0}", a);

    auto app = GGEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}

#endif