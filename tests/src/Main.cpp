#include "platypus/core/Application.h"
#include "platypus/core/Window.h"
#include "platypus/core/InputManager.h"
#include "platypus/core/Debug.h"


int main(int argc, const char** argv)
{
    platypus::Window window(800, 600, true, false, "Test");
    platypus::InputManager inputManager(&window);
    platypus::Application app(&window, &inputManager);

    app.run();

    return 0;
}
