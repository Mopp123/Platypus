#include "platypus/core/Application.h"
#include "TestScene.h"


int main(int argc, const char** argv)
{
    platypus::Application app("Platypus-test", 800, 600, true, false, new TestScene);
    app.run();

    return 0;
}
