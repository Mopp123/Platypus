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

    _entity1 = createEntity();
    float s = 300;
    create_gui_transform(
        _entity1,
        { 0, 0 },
        { s, s }
    );

    AssetManager& assetManager = Application::get_instance()->getAssetManager();
    Font* pFont = assetManager.loadFont("assets/fonts/Ubuntu-R.ttf", 16);

    GUIRenderable* pRenderable = create_gui_renderable(
        _entity1,
        { 1, 0, 0, 1 }
    );
    pRenderable->textureID = pFont->getTextureID();
    pRenderable->fontID = pFont->getID();
    pRenderable->layer = 0;

    pRenderable->text.resize(32);
    pRenderable->text = L"Web build testing, working?";

    // ---
    /*
    _entity2 = createEntity();
    float s2 = 300;
    create_gui_transform(
        _entity2,
        { 50, 250 },
        { s2, s2 }
    );

    Image* pTexImage = assetManager.loadImage("assets/test.png");
    TextureSampler texSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        1,
        0
    );
    Texture* pTexture2 = assetManager.createTexture(pTexImage->getID(), texSampler, 2);

    GUIRenderable* pRenderable2 = create_gui_renderable(
        _entity2,
        { 1, 0, 0, 1 }
    );
    pRenderable2->textureID = pTexture2->getID();
    pRenderable2->layer = 1;
    pRenderable2->textureOffset = { 1, 1 };


    // ---
    entityID_t entity3 = createEntity();
    float s3 = 200;
    create_gui_transform(
        entity3,
        { 325, 535 },
        { s3, s3 }
    );
    GUIRenderable* pRenderable3 = create_gui_renderable(
        entity3,
        { 1, 0, 0, 1 }
    );
    pRenderable3->layer = 1;
    */
}

void UITestScene::update()
{
    updateBase();

    InputManager& inputManager = Application::get_instance()->getInputManager();
    if (inputManager.isKeyDown(KeyName::KEY_0))
    {
        GUIRenderable* pRenderable1 = (GUIRenderable*)getComponent(_entity1, ComponentType::COMPONENT_TYPE_GUI_RENDERABLE);
        GUIRenderable* pRenderable2 = (GUIRenderable*)getComponent(_entity2, ComponentType::COMPONENT_TYPE_GUI_RENDERABLE);

        pRenderable1->layer = 0;
        pRenderable2->layer = 1;
    }
    else if (inputManager.isKeyDown(KeyName::KEY_1))
    {
        GUIRenderable* pRenderable1 = (GUIRenderable*)getComponent(_entity1, ComponentType::COMPONENT_TYPE_GUI_RENDERABLE);
        GUIRenderable* pRenderable2 = (GUIRenderable*)getComponent(_entity2, ComponentType::COMPONENT_TYPE_GUI_RENDERABLE);

        pRenderable1->layer = 1;
        pRenderable2->layer = 0;
    }
}
