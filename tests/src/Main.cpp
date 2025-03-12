#include "platypus/core/Application.h"
#include "TestScene.h"


int main(int argc, const char** argv)
{
    platypus::Application app(
        "Platypus-web-test",
        800,
        600,
        true,
        platypus::WindowMode::WINDOWED,
        new TestScene
    );
    app.run();
    return 0;
}
