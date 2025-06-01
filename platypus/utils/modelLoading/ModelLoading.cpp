#include "ModelLoading.h"
#include "platypus/core/Debug.h"
#include <unordered_map>
#include <algorithm>

// NOTE: GLTFVertexParsing and GLTFSkeletonParsing includes tinygltf as well
// and needs to be included before below defines and tinygltf include!
#include "GLTFVertexParsing.h"
#include "GLTFSkeletonParsing.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"

#include "GLTFFileUtils.h"


namespace platypus
{
    // NOTE:
    // Current limitations:
    //  * no skeleton loading
    //  * no material loading

    // TODO:
    //  * Skeleton loading
    //  * get simple sample files working from Khronos repo
    //      -> something was wrong with RiggedFigure, having something to do
    //      with multiple vertex attribs overlapping?
    bool load_gltf_model(
        const CommandPool& commandPool,
        const std::string& filepath,
        std::vector<MeshData>& outMeshes
    )
    {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF loader;
        std::string error;
        std::string warning;

        std::string ext = filepath.substr(filepath.find("."), filepath.size());
        bool ret = false;
        if (ext == ".glb")
            ret = loader.LoadBinaryFromFile(&gltfModel, &error, &warning, filepath); // for binary glTF(.glb)
        else if (ext == ".gltf")
            ret = loader.LoadASCIIFromFile(&gltfModel, &error, &warning, filepath);

        if (!warning.empty()) {
            Debug::log(
                "@load_gltf_model "
                "Warning while loading file: " + filepath + " "
                "tinygltf warning: " + warning,
                Debug::MessageType::PLATYPUS_WARNING
            );
        }

        if (!error.empty()) {
            Debug::log(
                "@load_gltf_model "
                "Error while loading file: " + filepath + " "
                "tinygltf error: " + error,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return false;
        }

        if (!ret) {
            Debug::log(
                "@load_gltf_model "
                "Error while loading file: " + filepath + " "
                "Failed to parse file ",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return false;
        }

        const tinygltf::Scene& gltfScene = gltfModel.scenes[gltfModel.defaultScene];
        const std::vector<tinygltf::Node>& modelNodes = gltfModel.nodes;

        Debug::log(
            "@load_gltf_model "
            "Processing glTF Model with total scene count = " + std::to_string(gltfModel.scenes.size()) + " "
            "total node count = " + std::to_string(modelNodes.size())
        );

        // NOTE: Atm not supporting multiple scenes!
        if (gltfModel.scenes.size() != 1)
        {
            Debug::log(
                "@load_gltf_model Multiple scenes not supported!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return false;
        }

        // Load meshes
        for (size_t i = 0; i < modelNodes.size(); ++i)
        {
            const tinygltf::Node& node = modelNodes[i];
            // NOTE: Could also loop just the gltfModel.meshes
            // BUT atm I want to get also the meshes' transform from their node
            if (node.mesh == -1)
                continue;

            tinygltf::Mesh& gltfMesh = gltfModel.meshes[node.mesh];
            Vector3f position;
            Quaternion rotation;
            Vector3f scale(1, 1, 1);
            if (node.translation.size() == 3)
                position = to_engine_vector3(node.translation);
            if (node.rotation.size() == 4)
                rotation = to_engine_quaternion(node.rotation);
            if (node.scale.size() == 3)
                scale = to_engine_vector3(node.scale);

            Matrix4f transformationMatrix = create_transformation_matrix(
                position,
                rotation,
                scale
            );

            std::vector<MeshBufferData> indexBuffers;
            MeshBufferData vertexBuffer;
            VertexBufferLayout vertexBufferLayout;
            if (!load_index_buffers(commandPool, gltfModel, gltfMesh, indexBuffers))
            {
                Debug::log(
                    "@load_gltf_model "
                    "Failed to load index buffers for mesh: " + std::to_string(i) + " "
                    "from file: " + filepath,
                    Debug::MessageType::PLATYPUS_ERROR
                );
            }
            // Don't support multiple index buffers for single mesh atm
            if (indexBuffers.size() != 1)
            {
                Debug::log(
                    "@load_gltf_model "
                    "Mesh had " + std::to_string(indexBuffers.size()) + " index buffers. "
                    "Currently supporting only single index buffer per mesh",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            if (!load_vertex_buffer(commandPool, gltfModel, gltfMesh, vertexBufferLayout, vertexBuffer))
            {
                Debug::log(
                    "@load_gltf_model "
                    "Failed to load vertex buffer for mesh: " + std::to_string(i) + " "
                    "from file: " + filepath,
                    Debug::MessageType::PLATYPUS_ERROR
                );
            }

            outMeshes.push_back({
                vertexBufferLayout,
                vertexBuffer,
                indexBuffers,
                transformationMatrix,
                {}
            });
        }

        // Indexing should follow above meshes indices
        for (size_t i = 0; i < gltfModel.skins.size(); ++i)
        {
            // Load skeleton (bind pose) (if found)
            Pose bindPose;
            // NOTE: Not sure is this skin index correct
            int rootJointNodeIndex = gltfModel.skins[0].joints[0];
            // Mapping from gltf joint node index to our pose struct's joint index
            std::unordered_map<int, int> nodeJointMapping;
            add_gltf_joint(
                gltfModel,
                bindPose,
                -1, // index to pose struct's parent joint. NOT glTF node index!
                rootJointNodeIndex,
                nodeJointMapping
            );
            outMeshes[i].bindPose = bindPose;

            // Load animations (if found)
        }

        return true;
    }
}
