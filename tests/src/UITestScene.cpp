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

    AssetManager& assetManager = Application::get_instance()->getAssetManager();
    Font* pFont = assetManager.loadFont("assets/fonts/Ubuntu-R.ttf", 16);

    InputManager& inputManager = Application::get_instance()->getInputManager();

    ui::Layout layout {
        { { ui::VType::PR, 0 } , { ui::VType::PR, 0 } }, // pos
        { ui::VType::PR, 50 }, // width
        { ui::VType::PR, 50 }, // height
        { 0.4f, 0.4f, 0.4f, 1.0f } // color
    };

    _ui.init(this, inputManager);

    ui::UIElement parentElem = _ui.createElement(layout);

    ui::Layout layout2 {
        { VPERCENT_T(50) , VPERCENT_T(50) }, // pos
        VPIXEL_T(100), // width
        VPIXEL_T(100), // height
        { 0.0f, 0.0f, 0.6f, 1.0f } // color
    };
    _ui.addChild(parentElem, layout2);
}

void UITestScene::update()
{
    updateBase();
}

void UITestScene::createBox(
    const Vector2f& pos,
    const Vector2f& scale,
    const Vector4f& color,
    uint32_t layer
)
{
    entityID_t boxEntity = createEntity();
    create_gui_transform(
        boxEntity,
        pos,
        scale
    );
    GUIRenderable* pBoxRenderable = create_gui_renderable(
        boxEntity,
        color
    );
    pBoxRenderable->layer = layer;
}

void UITestScene::createTextBox(
    const std::wstring& txt,
    const Vector2f& pos,
    const Vector2f& scale,
    const Vector4f& color,
    uint32_t layer,
    const Font* pFont
)
{
    createBox(pos, scale, color, layer);

    float padding = 5.0f;
    entityID_t textEntity = createEntity();
    create_gui_transform(
        textEntity,
        { pos.x + padding, pos.y + padding },
        { 1, 1 }
    );
    GUIRenderable* pTextRenderable = create_gui_renderable(
        textEntity,
        { 1, 1, 0, 1 }
    );
    pTextRenderable->textureID = pFont->getTextureID();
    pTextRenderable->fontID = pFont->getID();
    pTextRenderable->layer = layer + 1;

    pTextRenderable->text.resize(32);
    pTextRenderable->text = txt;
}
