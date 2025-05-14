#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class MeshTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

public:
    MeshTestScene();
    ~MeshTestScene();

    virtual void init();
    virtual void update();
};
