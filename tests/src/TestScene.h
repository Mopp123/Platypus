#pragma once

#include "platypus/core/Scene.h"

class TestScene : public platypus::Scene
{
public:
    TestScene();
    ~TestScene();
    virtual void init();
    virtual void update();
};
