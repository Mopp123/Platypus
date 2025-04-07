/*

TODO:
    *Create floor plane and texture it
    *Load tree and grass models
    *Position tree and grass entities randomly
*/

#pragma once

#include "platypus/core/Scene.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/utils/controllers/CameraController.h"

class TestScene : public platypus::Scene
{
private:
    platypus::CameraController _camController;
    entityID_t _camEntity = NULL_ENTITY_ID;

public:
    TestScene();
    ~TestScene();
    virtual void init();
    virtual void update();
};
