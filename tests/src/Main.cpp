#include "ShadowTestScene.hpp"
#include <thread>
#include <iostream>


void func()
{
    std::cout << "Hello from another thread\n";
}

int main(int argc, const char** argv)
{
    void (*pFunc)() = &func;
    std::thread t(pFunc);
    t.join();

    /*
    platypus::WindowMode windowMode = platypus::WindowMode::WINDOWED;
    #ifdef PLATYPUS_BUILD_WEB
        windowMode = platypus::WindowMode::WINDOWED_FIT_SCREEN;
    #endif

    platypus::Application app(
        "Engine-test",
        1024,
        768,
        true,
        windowMode,
        new ShadowTestScene
    );
    app.run();
    */
    return 0;
}
