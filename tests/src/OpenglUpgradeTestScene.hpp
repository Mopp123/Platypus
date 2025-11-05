#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.hpp"


class OpenglUpgradeTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

public:
    OpenglUpgradeTestScene();
    ~OpenglUpgradeTestScene();

    virtual void init();
    virtual void update();
};
