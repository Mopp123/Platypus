#include "UITestScene.hpp"
#include "platypus/ui/LayoutUI.h"
#include "platypus/ui/Text.h"
#include "platypus/ui/Button.h"

#include <iostream>


using namespace platypus;


class TestCharInputEvent : public CharInputEvent
{
public:
    std::string& inputStrRef;
    ui::UIElement* pInputElement = nullptr;
    ui::UIElement* pInputParentElement = nullptr;

    TestCharInputEvent(
        std::string& inputStrRef,
        ui::UIElement* pInputElement,
        ui::UIElement* pInputParentElement
    ) :
        inputStrRef(inputStrRef),
        pInputElement(pInputElement),
        pInputParentElement(pInputParentElement)
    {}

    virtual void func(unsigned int codepoint)
    {
        //inputStrRef += (wchar_t)codepoint;
        util::str::append_utf8((uint32_t)codepoint, inputStrRef);
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
        { 200, 400 }, // scale
        { 0.2f, 0.2f, 0.2f, 1.0f } // color
    };
    textBoxLayout.horizontalAlignment = ui::HorizontalAlignment::CENTER;
    textBoxLayout.verticalAlignment = ui::VerticalAlignment::CENTER;
    textBoxLayout.padding = { 10, 10 };
    textBoxLayout.wordWrap = ui::WordWrap::NORMAL;

    _pInputBoxElement = ui::add_container(_ui, nullptr, textBoxLayout, true);

    std::string testStr = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse sodales sollicitudin eros faucibus accumsan. Donec pharetra nec massa ut volutpat. Etiam vestibulum velit tincidunt lorem interdum efficitur consequat facilisis felis. Etiam aliquam rutrum mattis. Morbi bibendum augue aliquam congue gravida. Donec dictum arcu vel posuere sollicitudin. Nulla facilisi. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Morbi ligula mauris, condimentum eget ultrices ut, dictum id urna. Curabitur feugiat nibh nunc, fermentum eleifend nulla bibendum sed. Aenean vitae felis sit amet est pretium placerat in vitae quam. Donec vulputate dictum lacinia. Duis fermentum egestas consectetur. Aliquam sit amet efficitur erat, vitae lobortis arcu. Duis rutrum lobortis odio vitae varius. Suspendisse fermentum, justo vestibulum condimentum vulputate, orci dui posuere dolor, at fermentum quam mauris eu nibh. Curabitur quis blandit risus. Morbi non mauris id leo vehicula tempor ac id sapien. Nulla facilisi. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Fusce commodo lectus nec diam molestie, nec pharetra quam pellentesque. Nulla facilisi. Sed vitae condimentum turpis. Fusce finibus dui est, et venenatis odio dictum ut. Morbi semper semper volutpat. Quisque sit amet placerat quam. Vivamus ex dolor, blandit eget enim sit amet, facilisis aliquam augue. Sed et faucibus mauris. Proin quis interdum dolor. Phasellus facilisis turpis lectus, eu accumsan neque imperdiet vitae. Cras a neque urna. Cras aliquet rutrum semper. Vivamus at pharetra nisl, quis finibus risus. Praesent vitae vulputate elit. Nunc eu magna augue. Mauris vel pretium mauris. Pellentesque nibh magna, posuere eget fermentum non, blandit vitae tellus. Vivamus pretium felis et sollicitudin porttitor. Quisque quis ultrices metus. Praesent turpis nulla, luctus at gravida ut, gravida et leo. Praesent malesuada accumsan odio, quis luctus velit mattis et. Ut sit amet ex urna. Nam luctus nunc eu arcu consectetur sodales. Pellentesque tincidunt sem sit amet purus semper, id mollis lectus malesuada. In iaculis rutrum facilisis. Morbi dignissim ac purus in lacinia. Aliquam non ex bibendum orci condimentum posuere. Aenean quis ligula lacus. Quisque nulla augue, molestie at sagittis in, ornare sit amet elit. Ut congue dui id mauris bibendum pretium. Maecenas consectetur diam eu sapien ornare convallis. Proin feugiat consectetur turpis congue porttitor. In ullamcorper nisl non lobortis accumsan. Ut porttitor sollicitudin felis vel volutpat. Aliquam erat volutpat. Morbi blandit mi ac urna fermentum euismod. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Cras vitae bibendum odio. Donec sodales sit amet dolor sed elementum. Donec nec scelerisque libero. Cras sodales, odio quis auctor gravida, risus nisl euismod erat, id mattis ex mauris at ipsum. Donec posuere finibus neque ut aliquam. Praesent sodales lorem enim, non efficitur justo finibus et. Suspendisse hendrerit id ipsum a vulputate. Morbi a nunc at nunc efficitur mollis. Nunc eget nunc ac purus gravida vulputate nec eget urna. Phasellus tortor erat, lobortis sit amet luctus id, sodales eget mauris. Etiam pharetra, tortor nec tempus elementum, velit est consectetur libero, consectetur condimentum ipsum justo et nisi. Aliquam ac mauris magna. Aenean non cursus ipsum. Cras vitae egestas felis, mattis auctor dolor. Aliquam sagittis, lacus eu feugiat ultrices, sapien nibh dictum neque, vitae convallis nunc risus non mauris. Vestibulum at gravida enim. Praesent faucibus euismod neque varius vulputate. Vestibulum aliquam nisl justo, et suscipit massa mattis sed. Sed vel massa sodales ex accumsan viverra ac quis nunc. Cras pharetra nisi vitae pulvinar mollis. Integer non tincidunt lectus. Phasellus dictum, nibh sed hendrerit venenatis, est ipsum efficitur leo, non mattis nibh orci vitae ipsum. Morbi ut ex id nisl tincidunt commodo a laoreet elit. Curabitur vel elit facilisis nunc egestas posuere.";

    _pInputTextElement = ui::add_text_element(
        _ui,
        _pInputBoxElement,
        testStr,
        { 1, 1, 1, 1 },
        pFont
    );

    //inputManager.addCharInputEvent(
    //    new TestCharInputEvent(
    //        _inputStr,
    //        _pInputTextElement,
    //        _pInputBoxElement
    //    )
    //);

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
            util::str::pop_back_utf8(_inputStr);
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
    const std::string& txt,
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
