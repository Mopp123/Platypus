#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.hpp"
#include "platypus/ecs/Entity.h"


class ShadowTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

    platypus::Mesh* _pTerrainMesh = nullptr;
    platypus::Material* _pTerrainMaterial = nullptr;

    std::vector<float> _heightmap1;
    std::vector<float> _heightmap2;

    entityID_t _framebufferDebugEntity = NULL_ENTITY_ID;
    platypus::Texture* _pTestTexture1 = nullptr;
    platypus::Texture* _pTestTexture2 = nullptr;
    platypus::Texture* _pFramebufferDebugTexture = NULL_ID;

public:
    ShadowTestScene();
    ~ShadowTestScene();

    virtual void init();
    virtual void update();
};
