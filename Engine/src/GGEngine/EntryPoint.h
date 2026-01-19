#pragma once

#ifdef GG_PLATFORM_WINDOWS

extern GGEngine::Application* GGEngine::CreateApplication();



int main(int argc, char** argv) {
    auto app = GGEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}

#endif