#pragma once

#include "BaseScene.hpp"

class UITestScene : public BaseScene
{
private:
    platypus::ui::LayoutUI _ui;

    std::string _inputStr;
    platypus::ui::UIElement* _pInputBoxElement = nullptr; // parent of _pInputTextElement
    platypus::ui::UIElement* _pInputTextElement = nullptr;

public:
    UITestScene();
    ~UITestScene();
    virtual void init();
    virtual void update();

private:
    void createBox(
        const platypus::Vector2f& pos,
        const platypus::Vector2f& scale,
        const platypus::Vector4f& color,
        uint32_t layer
    );

    void createTextBox(
        const std::string& txt,
        const platypus::Vector2f& pos,
        const platypus::Vector2f& scale,
        const platypus::Vector4f& color,
        uint32_t layer,
        const platypus::Font* pFont
    );
};
