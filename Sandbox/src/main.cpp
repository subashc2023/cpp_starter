#include "GGEngine.h"

class Sandbox : public GGEngine::Application {
public:
    Sandbox() {
    }
    ~Sandbox() {
    }
};

GGEngine::Application* GGEngine::CreateApplication() {
    return new Sandbox();
}