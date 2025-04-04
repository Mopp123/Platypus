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

class TestScene : public platypus::Scene
{
public:
    entityID_t testEntity = NULL_ENTITY_ID;
    entityID_t testEntity2 = NULL_ENTITY_ID;

    TestScene();
    ~TestScene();
    virtual void init();
    virtual void update();
};
