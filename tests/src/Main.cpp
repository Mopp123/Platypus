#include "platypus/Platypus.h"
#include "UITestScene.h"
#include "SkinnedMeshTestScene.h"


int main(int argc, const char** argv)
{
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
        new SkinnedMeshTestScene
    );
    app.run();
    return 0;
}
