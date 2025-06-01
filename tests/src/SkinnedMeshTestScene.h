#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class SkinnedMeshTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

public:
    SkinnedMeshTestScene();
    ~SkinnedMeshTestScene();

    virtual void init();
    virtual void update();
};
