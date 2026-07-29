#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.hpp"


class MeshSerializationTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

public:
    MeshSerializationTestScene();
    ~MeshSerializationTestScene();

    virtual void init();
    virtual void update();
};
