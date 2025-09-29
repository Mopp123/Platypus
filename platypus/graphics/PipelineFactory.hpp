#pragma once

#include "Pipeline.h"
#include "platypus/assets/Mesh.h"
#include "platypus/assets/Material.h"

namespace platypus
{
    Pipeline* create_material_pipeline(
        const VertexBufferLayout& meshVertexBufferLayout,
        bool instanced,
        bool skinned,
        Material* pMaterial
    );

    Pipeline* create_terrain_material_pipeline(
        Material* pMaterial
    );
}
