#include "ShadowTestScene.hpp"
#include <thread>
#include <iostream>

#include <future>
#include "emscripten.h"


std::future<int> my_future;
bool s_running = false;
static bool s_start = true;


int func()
{
    std::cout << "Thread running...\n";
    return 1;
}

void main_loop() {
    if (s_start)
    {
        my_future = std::async(std::launch::async, func);
        s_running = true;
        s_start = false;
    }
    if (s_running) {
        // Poll the future: check its status for 0 milliseconds
        auto status = my_future.wait_for(std::chrono::milliseconds(0));

        if (status == std::future_status::ready) {
            int result = my_future.get(); // Retrieve the value
            s_running = false;
            printf("Thread finished with: %d\n", result);
        }
    }

    // ... render your frame ...
}

int main(int argc, const char** argv)
{
    emscripten_set_main_loop(main_loop, 0, true);

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
