#include "GGEngine.h"

class ExampleLayer : public GGEngine::Layer
{
public:
    ExampleLayer() : Layer("ExampleLayer") 
    {
    }

    void OnUpdate() override
    {
    }

    void OnImGuiRender() override
    {
        // Empty for now to test basic ImGui
    }

    void OnEvent(GGEngine::Event& event) override
    {
    }
};

class Sandbox : public GGEngine::Application 
{
public:
    Sandbox() 
    {
        PushLayer(new ExampleLayer());
    }
    ~Sandbox() 
    {
    }
};

GGEngine::Application* GGEngine::CreateApplication() {
    return new Sandbox();
}