#include "GGEngine.h"

class EditorLayer : public GGEngine::Layer
{
public:
    EditorLayer() : Layer("EditorLayer") 
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

class Editor : public GGEngine::Application 
{
public:
    Editor() 
    {
        PushLayer(new EditorLayer());
    }
    ~Editor() 
    {
    }
};

GGEngine::Application* GGEngine::CreateApplication() {
    return new Editor();
}
