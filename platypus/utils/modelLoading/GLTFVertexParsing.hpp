#pragma once

#include "platypus/graphics/Buffers.hpp"
#include "RawMeshData.hpp"


// Nasty shit to be able to use here...
namespace tinygltf
{
    class Model;
    struct Mesh;
}

namespace platypus
{
    bool load_index_buffers(
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh,
        std::vector<MeshBufferData>& outBufferData
    );


    // NOTE:
    // Current limitations!
    // * Expecting to have only a single set of primitives for single mesh!
    //  -> if thats not the case shit gets fucked!
    bool load_vertex_buffer(
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh,
        VertexBufferLayout& outLayout,
        MeshBufferData& outBufferData
    );
}
