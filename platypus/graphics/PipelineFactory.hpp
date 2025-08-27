#pragma once

#include "Pipeline.h"
#include "platypus/assets/Mesh.h"
#include "platypus/assets/Material.h"

namespace platypus
{
    Pipeline* create_material_pipeline(
        const Mesh* pMesh,
        bool instanced,
        bool skinned,
        Material* pMaterial
    );
}
