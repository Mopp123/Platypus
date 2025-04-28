#include "UITestScene.h"
#include "platypus/ui/LayoutUI.h"

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
    _ui.init(this, inputManager);

    // NOTE: This "base requirement" is fucking gay for being able to anchor at some pos
    // -> make it possible to put parent like top,left up or bottom aligned or something!
    ui::Layout baseLayout;
    baseLayout.fullscreen = true;
    baseLayout.horizontalAlignment = ui::HorizontalAlignment::CENTER;
    baseLayout.verticalAlignment = ui::VerticalAlignment::BOTTOM;
    baseLayout.expandElements = ui::ExpandElements::RIGHT;

    // TODO: Some kind of "user id" thing to access created "container" afterwards

    ui::Layout layout {
        { 0, 0 }, // pos
        { 400, 200 }, // scale
        { 0.4f, 0.4f, 0.4f, 1.0f } // color
    };
    layout.padding = { 10, 10 };
    layout.elementGap = 10;
    layout.horizontalAlignment = ui::HorizontalAlignment::LEFT;
    layout.verticalAlignment = ui::VerticalAlignment::TOP;
    layout.expandElements = ui::ExpandElements::RIGHT;

    for (int i = 0; i < 4; ++i)
    {
        layout.children.push_back({
            { 0, 0 }, // pos
            { 50, 50 }, // scale
            { 0.4f, 0.0f, 0.4f, 1.0f } // color
        });
    }

    baseLayout.children = { layout };

    _ui.create(
        baseLayout,
        nullptr,
        NULL_ENTITY_ID
    );

    /*
    TODO: Make the shit work rather like below!

    UIElement panel1 = _ui.createContainer(layout);
    UIElement text = panel1.addText("Testing");

    UIElement panel1Child1 = panel1.addContainer(layout2);
    UIElement text2 = panel1Child1.addText("Testing child panel");
    UIElement button = panel1Child1.addButton("Test Button", SIZE_AUTO);
    UIElement button2 = panel1Child1.addButton("Test Button2", SIZE_FIXED, 100);

    UIElement panel1Child2 = panel1.addContainer(layout3);
    UIElement image = panel1Child2.addImage(pTestTexture);
    */
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
