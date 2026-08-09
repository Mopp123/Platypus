#pragma once

#include "platypus/utils/AnimationDataUtils.hpp"
#include "RawMeshData.hpp"
#include <vector>
#include <string>


namespace platypus
{
    // NOTE:
    //  *Breaks if multiple skeletons for single mesh
    //  *Breaks if multiple different skeletons and animations for different meshes
    //  *Breaks if animations exists and model's each mesh doesn't contain armature
    //
    // TODO: Make less dumb
    bool load_gltf_model(
        const std::string& filepath,
        std::vector<MeshData>& outMeshes,
        std::vector<SkeletonData>& outSkeletons
    );
}
