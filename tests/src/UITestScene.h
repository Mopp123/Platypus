#pragma once

#include "BaseScene.h"

class UITestScene : public BaseScene
{
private:
    platypus::ui::LayoutUI _ui;

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
        const std::wstring& txt,
        const platypus::Vector2f& pos,
        const platypus::Vector2f& scale,
        const platypus::Vector4f& color,
        uint32_t layer,
        const platypus::Font* pFont
    );
};
