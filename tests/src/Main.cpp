#include "ShadowTestScene.hpp"
#include <thread>
#include <iostream>

#include <future>
#include <vector>
#include "emscripten.h"


static std::future<int> s_future;
static std::future<int> s_future2;
bool s_running = false;
static bool s_start = true;

class SafeContainer
{
public:
    std::mutex mutex;
    std::vector<int> data;
    SafeContainer() {}

    void add(int val)
    {
        std::lock_guard<std::mutex> lock(mutex);
        data.push_back(val);
    }
};

static SafeContainer s_container;

int func()
{
    std::cout << "Thread started...\n";
    for (int i = 0; i < 100; ++i)
    {
        std::cout << "adding to container...\n";
        s_container.add(i);
    }

    return 1;
}

int func2()
{
    std::cout << "Thread started...\n";
    for (int i = 100; i < 200; ++i)
        s_container.add(i);

    return 1;
}

void main_loop()
{
    if (s_start)
    {
        s_future = std::async(std::launch::async, func);
        s_future2 = std::async(std::launch::async, func2);
        s_running = true;
        s_start = false;
    }
    if (s_running)
    {
        std::cout << "Waiting for threads to finish...\n";
        // Poll the future: check its status for 0 milliseconds
        auto status = s_future.wait_for(std::chrono::milliseconds(0));
        auto status2 = s_future2.wait_for(std::chrono::milliseconds(0));

        if (status == std::future_status::ready && status2 == std::future_status::ready)
        {
            int result = s_future.get();
            int result2 = s_future2.get();
            s_running = false;
            std::cout << "Threads finished. Result container:\n";
            for (size_t i = 0; i < s_container.data.size(); ++i)
            {
                std::cout << "[" << i << "] = " << s_container.data[i] << std::endl;
            }
        }
    }
    // ... render your frame ...
}

int main(int argc, const char** argv)
{
    emscripten_set_main_loop(main_loop, 0, true);
    std::cout << "Some main code there...\n";

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
