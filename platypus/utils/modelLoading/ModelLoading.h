#pragma once

#include "platypus/utils/AnimationDataUtils.h"
#include "RawMeshData.h"
#include <vector>
#include <string>


namespace platypus
{
    // TODO: Make less dumb
    bool load_gltf_model(
        const std::string& filepath,
        std::vector<MeshData>& outMeshes,
        std::vector<Pose>& outBindPoses,
        std::vector<KeyframeAnimationData>& outAnimations
    );
}
