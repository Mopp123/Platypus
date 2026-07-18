#include "UITestScene.hpp"


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
    environmentProperties.clearColor = { 0, 0, 0, 1 };

    AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
    Font* pFont = pAssetManager->loadFont("assets/fonts/Ubuntu-R.ttf", 16);

    InputManager& inputManager = Application::get_instance()->getInputManager();
}


void UITestScene::update()
{
    updateBase();
}
