#include "TestScene.h"
#include "platypus/core/Debug.h"

using namespace platypus;

TestScene::TestScene()
{
    Debug::log("___TEST___created test scene!");
}

TestScene::~TestScene()
{
    Debug::log("___TEST___destroyed test scene!");
}

void TestScene::init()
{
    Debug::log("___TEST___initialized test scene!");
}

void TestScene::update()
{
}
