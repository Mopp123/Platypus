#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class TerrainTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

    platypus::TerrainMesh* _pTerrainMesh = nullptr;

    std::vector<float> _heightmap1;
    std::vector<float> _heightmap2;

public:
    TerrainTestScene();
    ~TerrainTestScene();

    virtual void init();
    virtual void update();
};
