#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/CommandBuffer.h"
#include "RawMeshData.h"


// Nasty shit to be able to use here...
namespace tinygltf
{
    class Model;
    class Mesh;
}

namespace platypus
{
    bool load_index_buffers(
        const CommandPool& commandPool,
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh,
        std::vector<MeshBufferData>& outBufferData
    );


    // NOTE:
    // Current limitations!
    // * Expecting to have only a single set of primitives for single mesh!
    //  -> if thats not the case shit gets fucked!
    bool load_vertex_buffer(
        const CommandPool& commandPool,
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh,
        VertexBufferLayout& outLayout,
        MeshBufferData& outBufferData
    );
}
