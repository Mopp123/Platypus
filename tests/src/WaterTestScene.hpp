#pragma once

#include "BaseScene.hpp"
#include "platypus/Platypus.h"

class WaterTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;
    platypus::Material* _pWaterMaterial = nullptr;

    entityID_t _framebufferDebugEntity = NULL_ENTITY_ID;

public:
    WaterTestScene();
    ~WaterTestScene();

    virtual void init();
    virtual void update();
};
