#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class TerrainTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

public:
    TerrainTestScene();
    ~TerrainTestScene();

    virtual void init();
    virtual void update();
};
