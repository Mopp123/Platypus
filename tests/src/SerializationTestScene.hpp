#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.hpp"


class SerializationTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

public:
    SerializationTestScene();
    ~SerializationTestScene();

    virtual void init();
    virtual void update();
};
