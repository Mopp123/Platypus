#pragma once

#include "Pipeline.h"
#include "platypus/assets/Mesh.h"
#include "platypus/assets/Material.h"
#include "platypus/assets/TerrainMaterial.hpp"

namespace platypus
{
    Pipeline* create_material_pipeline(
        const Mesh* pMesh,
        bool instanced,
        bool skinned,
        Material* pMaterial
    );

    Pipeline* create_terrain_material_pipeline(
        TerrainMaterial* pMaterial
    );
}
