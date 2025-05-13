#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class MeshTestScene : public BaseScene
{
public:
    MeshTestScene();
    ~MeshTestScene();

    virtual void init();
    virtual void update();
};
