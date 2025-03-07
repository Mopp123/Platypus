//#include "platypus/core/Application.h"
//#include "TestScene.h"

// NOTE: Atm just testing web implementations individually

#include "platypus/core/Window.h"
#include "platypus/core/InputManager.h"
#include "platypus/graphics/Context.h"

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Buffers.h"

#include <emscripten.h>


using namespace platypus;

void update()
{
}

int main(int argc, const char** argv)
{
    const std::string appName = "Platypus-web-test";
    Window window(
      appName,
      800,
      600,
      false, // resizable
      WindowMode::WINDOWED_FIT_SCREEN
    );
    InputManager inputManager(window);
    Context context(appName.c_str(), &window);

    std::vector<float> vertices =
    {
        0.5f, 0.25f, 1.0f,
        0.5f, 0.25f, 1.0f,
        0.5f, 0.25f, 1.0f,
    };

    CommandPool commandPool;

    Buffer* pBuffer = new Buffer(
        commandPool,
        vertices.data(),
        sizeof(float) * 3,
        3,
        BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
        BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
    );

    emscripten_set_main_loop(update, 0, 1);
    /*
    platypus::Application app("Platypus-web-test", 800, 600, true, false, new TestScene);
    app.run();
    */
    return 0;
}
