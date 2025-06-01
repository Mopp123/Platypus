#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class MaterialTestScene : public BaseScene
{
private:
    platypus::ui::LayoutUI _ui;
    platypus::CameraController _camController;

    entityID_t _boxEntity;

public:
    MaterialTestScene();
    ~MaterialTestScene();

    virtual void init();
    virtual void update();
};
