#pragma once

#include "platypus/core/Scene.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"

#include "platypus/assets/Texture.h"


class TestScene : public platypus::Scene
{
public:
    entityID_t testEntity = NULL_ENTITY_ID;
    platypus::TextureSampler* pTextureSampler = nullptr;

    TestScene();
    ~TestScene();
    virtual void init();
    virtual void update();
};
