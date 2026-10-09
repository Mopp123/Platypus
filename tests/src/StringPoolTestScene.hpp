#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.hpp"


class StringPoolTestScene : public BaseScene
{
private:
    platypus::StringPool _stringPool;

public:
    StringPoolTestScene();
    ~StringPoolTestScene();

    virtual void init();
    virtual void update();
};
