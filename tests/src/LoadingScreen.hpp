#pragma once


#include "platypus/Platypus.h"
#include "BaseScene.hpp"


class LoadingScreen : public BaseScene
{
private:
    platypus::ui::LayoutUI _ui;

public:
    LoadingScreen();
    ~LoadingScreen();

    virtual void init();
    virtual void update();
};
