#pragma once

#include "BaseScene.h"

class UITestScene : public BaseScene
{
private:
    entityID_t _entity1;
    entityID_t _entity2;

public:
    UITestScene();
    ~UITestScene();
    virtual void init();
    virtual void update();
};
