#pragma once

#include "platypus/utils/AnimationDataUtils.hpp"
#include "RawMeshData.hpp"
#include <vector>
#include <string>


namespace platypus
{
    // TODO: Make less dumb
    bool load_gltf_model(
        const std::string& filepath,
        std::vector<MeshData>& outMeshes,
        std::vector<KeyframeAnimationData>& outAnimations
    );
}
