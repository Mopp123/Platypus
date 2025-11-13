#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.hpp"
#include "platypus/ecs/Entity.h"


class TerrainTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

    platypus::Mesh* _pTerrainMesh = nullptr;
    platypus::Material* _pTerrainMaterial = nullptr;

    platypus::Material* _pMeshMaterial = nullptr;

    std::vector<float> _heightmap1;
    std::vector<float> _heightmap2;

public:
    TerrainTestScene();
    ~TerrainTestScene();

    virtual void init();
    virtual void update();
};
