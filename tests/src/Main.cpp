//#include "platypus/core/Application.h"
//#include "TestScene.h"

// NOTE: Atm just testing web implementations individually

#include "platypus/core/Window.h"
#include "platypus/graphics/Context.h"

using namespace platypus;

int main(int argc, const char** argv)
{
    const std::string appName = "Platypus-web-test";
    Window window(
      appName,
      800,
      600,
      false, // resizable
      false // fullscreen
    );
    Context context(appName.c_str(), &window);
    /*
    platypus::Application app("Platypus-web-test", 800, 600, true, false, new TestScene);
    app.run();
    */
    return 0;
}
