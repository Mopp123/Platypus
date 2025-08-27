#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/CommandBuffer.h"
#include "platypus/utils/Maths.h"
#include "platypus/utils/AnimationDataUtils.h"
#include "RawMeshData.h"
#include "platypus/Common.h"
#include <vector>
#include <string>


namespace platypus
{
    // TODO: Make less dumb
    bool load_gltf_model(
        const std::string& filepath,
        std::vector<MeshData>& outMeshes,
        std::vector<Pose>& outBindPoses,
        std::vector<std::vector<Pose>>& outAnimations
    );
}
