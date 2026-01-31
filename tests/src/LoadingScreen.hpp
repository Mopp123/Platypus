#pragma once


#include "platypus/Platypus.h"
#include "BaseScene.hpp"


class LoadingScreen : public BaseScene
{
private:
    platypus::ui::LayoutUI _ui;
    platypus::ui::UIElement* _pBoxElement = nullptr;
    platypus::ui::UIElement* _pStatusTextElement = nullptr;
    platypus::ui::UIElement* _pProgressBarElement = nullptr;

    uint32_t _maxProgress = 0;
    float _maxVisualWidth = 0.0f;
    float _progressBarHeight = 5.0f;

    std::unordered_map<std::string, std::string> _texturePaths;
    std::unordered_map<std::string, platypus::Texture*> _textures;

public:
    LoadingScreen();
    ~LoadingScreen();

    virtual void init();
    virtual void update();

private:
    void setProgress(uint32_t progress);
};
