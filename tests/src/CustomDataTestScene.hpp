#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.hpp"


class CustomDataTestScene : public BaseScene
{
public:
    CustomDataTestScene();
    ~CustomDataTestScene();

    virtual void init();
    virtual void update();
};
