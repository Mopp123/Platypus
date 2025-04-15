#include "UITestScene.h"

using namespace platypus;


UITestScene::UITestScene()
{
}

UITestScene::~UITestScene()
{
}

void UITestScene::init()
{
    initBase();

    entityID_t entity = createEntity();

    Transform* pTransform = createTransform(
        entity,
        { 0, 0, 0 },
        { { 0, 0, 1 }, 0},
        { 1, 1, 1 }
    );

    createGUIRenderable(
        entity,
        { 1, 0, 0, 1 }
    );
}

void UITestScene::update()
{
    updateBase();
}
