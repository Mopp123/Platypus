#include "UITestScene.hpp"
#include "platypus/ui/LayoutUI.h"
#include "platypus/ui/Text.h"
#include "platypus/ui/Button.h"
#include "MaterialTestScene.hpp"

#include <iostream>


using namespace platypus;


class TestCharInputEvent : public CharInputEvent
{
public:
    std::wstring& inputStrRef;
    ui::UIElement* pInputElement = nullptr;
    ui::UIElement* pInputParentElement = nullptr;

    TestCharInputEvent(
        std::wstring& inputStrRef,
        ui::UIElement* pInputElement,
        ui::UIElement* pInputParentElement
    ) :
        inputStrRef(inputStrRef),
        pInputElement(pInputElement),
        pInputParentElement(pInputParentElement)
    {}

    virtual void func(unsigned int codepoint)
    {
        inputStrRef += (wchar_t)codepoint;
        ui::set_text(pInputElement, pInputParentElement, inputStrRef);
    }
};


class MouseEnterTest : public ui::UIElement::MouseEnterEvent
{
public:
    virtual void func(int x, int y)
    {
        Debug::log("___TEST___ENTER!");
    }
};

class MouseOverTest : public ui::UIElement::MouseOverEvent
{
public:
    virtual void func(int x, int y)
    {
        Debug::log("___TEST___OVER!");
    }
};

class MouseExitTest : public ui::UIElement::MouseExitEvent
{
public:
    virtual void func(int x, int y)
    {
        Debug::log("___TEST___EXIT!");
    }
};

class OnClickTest : public ui::UIElement::OnClickEvent
{
public:
    int index = 0;
    OnClickTest(int i) : index(i) {}
    virtual void func(MouseButtonName button, InputAction action)
    {
        if (action != InputAction::RELEASE)
        {
            Application::get_instance()->getSceneManager().assignNextScene(new MaterialTestScene);
            Debug::log("___TEST___ON CLICK: " + std::to_string(index));
        }
    }
};


UITestScene::UITestScene()
{
}

UITestScene::~UITestScene()
{
}

void UITestScene::init()
{
    initBase();

    AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
    Font* pFont = pAssetManager->loadFont("assets/fonts/Ubuntu-R.ttf", 16);

    InputManager& inputManager = Application::get_instance()->getInputManager();
    _ui.init(this, inputManager);

    // TODO: Some kind of "user id" thing to access created "container" afterwards
    /*
    ui::Layout layout {
        { 0, 0 }, // pos
        { 400, 200 }, // scale
        { 0.2f, 0.2f, 0.2f, 1.0f } // color
    };
    layout.padding = { 10, 10 };
    layout.elementGap = 10;

    layout.horizontalAlignment = ui::HorizontalAlignment::RIGHT;
    layout.verticalAlignment = ui::VerticalAlignment::CENTER;

    layout.horizontalContentAlignment = ui::HorizontalAlignment::LEFT;
    layout.verticalContentAlignment = ui::VerticalAlignment::TOP;

    layout.expandElements = ui::ExpandElements::DOWN;


    ui::UIElement* pContainer = ui::add_container(_ui, nullptr, layout, true);

    ui::UIElement* pButton = ui::add_button_element(
        _ui,
        pContainer,
        L"Test button",
        pFont
    );
    pButton->_pOnClickEvent = new OnClickTest(0);

    ui::UIElement* pButton2 = ui::add_button_element(
        _ui,
        pContainer,
        L"Öääö scand test",
        pFont
    );
    */


    // Testing creating some text box thing
    ui::Layout textBoxLayout {
        { 0, 0 }, // pos
        { 400, 200 }, // scale
        { 0.2f, 0.2f, 0.2f, 1.0f } // color
    };
    textBoxLayout.horizontalAlignment = ui::HorizontalAlignment::CENTER;
    textBoxLayout.verticalAlignment = ui::VerticalAlignment::BOTTOM;
    textBoxLayout.padding = { 10, 10 };
    textBoxLayout.wordWrap = ui::WordWrap::NORMAL;

    _pInputBoxElement = ui::add_container(_ui, nullptr, textBoxLayout, true);
    _pInputTextElement = ui::add_text_element(
        _ui,
        _pInputBoxElement,
        L"",
        { 1, 1, 1, 1 },
        pFont
    );

    inputManager.addCharInputEvent(
        new TestCharInputEvent(
            _inputStr,
            _pInputTextElement,
            _pInputBoxElement
        )
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
    Debug::log("___TEST___GUI TEST INIT! Created " + std::to_string(getEntities().size()) + " entities.");
}


static int s_TEST_backspaceState = 0;
void UITestScene::update()
{
    updateBase();

    InputManager& inputManager = Application::get_instance()->getInputManager();
    if (inputManager.isKeyDown(KeyName::KEY_BACKSPACE))
        s_TEST_backspaceState += 1;
    else
        s_TEST_backspaceState = 0;

    if (s_TEST_backspaceState == 1)
    {
        if (!_inputStr.empty())
        {
            _inputStr.pop_back();
            ui::set_text(_pInputTextElement, _pInputBoxElement, _inputStr);
        }
    }
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
