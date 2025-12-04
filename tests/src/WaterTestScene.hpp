#pragma once

#include "BaseScene.hpp"
#include "platypus/Platypus.h"

class WaterTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;
    platypus::Material* _pWaterMaterial = nullptr;

public:
    WaterTestScene();
    ~WaterTestScene();

    virtual void init();
    virtual void update();
};
