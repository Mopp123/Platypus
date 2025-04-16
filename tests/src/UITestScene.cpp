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

    float s = 0.5f;
    Transform* pTransform = createTransform(
        entity,
        { 0, 0, -0.1f },
        { { 0, 0, 1 }, 0},
        { s, s, 0 }
    );

    AssetManager& assetManager = Application::get_instance()->getAssetManager();
    Font* pFont = assetManager.loadFont("assets/fonts/Ubuntu-R.ttf", 16);

    GUIRenderable* pRenderable = createGUIRenderable(
        entity,
        { 1, 0, 0, 1 }
    );
    pRenderable->textureID = pFont->getTextureID();
}

void UITestScene::update()
{
    updateBase();
}
