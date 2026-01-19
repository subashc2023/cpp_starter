#include "GGEngine.h"

class Editor : public GGEngine::Application {
public:
    Editor() {
    }
    ~Editor() {
    }
};

GGEngine::Application* GGEngine::CreateApplication() {
    return new Editor();
}