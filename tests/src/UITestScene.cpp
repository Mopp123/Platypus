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

    ui::Layout layout {
        { VPIXEL_T(200) , VPIXEL_T(100) }, // pos
        { ui::VType::PR, 50 }, // width
        { ui::VType::PR, 50 }, // height
        { 0.4f, 0.4f, 0.4f, 1.0f } // color
    };

    layout.paddingX = VPIXEL_T(5);
    layout.paddingY = VPIXEL_T(5);

    layout.horizontalAlignment = ui::HorizontalAlignment::LEFT;
    layout.verticalAlignment = ui::VerticalAlignment::TOP;
    layout.expandElements = ui::ExpandElements::DOWN;

    ui::Layout layout2 {
        { VPERCENT_T(0) , VPERCENT_T(0) }, // pos
        VPERCENT_T(60), // width
        VPERCENT_T(60), // height
        { 0.0f, 0.0f, 0.6f, 1.0f } // color
    };
    layout2.paddingX = VPIXEL_T(5);
    layout2.paddingY = VPIXEL_T(6);

    layout2.horizontalAlignment = ui::HorizontalAlignment::RIGHT;
    layout2.verticalAlignment = ui::VerticalAlignment::TOP;
    layout2.elementGap = VPIXEL_T(10);
    layout2.expandElements = ui::ExpandElements::LEFT;

    ui::Layout layout3 {
        { VPERCENT_T(0) , VPERCENT_T(0) }, // pos
        VPIXEL_T(120), // width
        VPIXEL_T(30), // height
        { 0.6f, 0.0f, 0.6f, 1.0f } // color
    };
    ui::Layout layout4 {
        { VPERCENT_T(0) , VPERCENT_T(0) }, // pos
        VPIXEL_T(120), // width
        VPIXEL_T(30), // height
        { 0.6f, 0.6f, 0.0f, 1.0f } // color
    };
    ui::Layout layout5 {
        { VPERCENT_T(0) , VPERCENT_T(0) }, // pos
        VPIXEL_T(100), // width
        VPIXEL_T(75), // height
        { 0.6f, 0.6f, 0.6f, 1.0f } // color
    };

    layout2.children = { layout3, layout4, layout5 };

    layout.children = { layout2 };

    _ui.create(
        layout,
        nullptr,
        NULL_ENTITY_ID
    );
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
