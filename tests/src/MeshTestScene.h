#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class MeshTestScene : public BaseScene
{
private:
    platypus::ui::LayoutUI _ui;
    platypus::CameraController _camController;

    entityID_t _boxEntity;

public:
    MeshTestScene();
    ~MeshTestScene();

    virtual void init();
    virtual void update();
};
