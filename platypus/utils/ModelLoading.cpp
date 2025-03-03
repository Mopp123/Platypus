#include "ModelLoading.h"
#include "platypus/Common.h"
#include "platypus/core/Debug.h"
#include "Maths.h"
#include <unordered_map>
#include <algorithm>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION

#include "tiny_gltf.h"

#define GLTF_POINTS         0x0
#define GLTF_LINES          0x0001
#define GLTF_LINE_LOOP      0x0002
#define GLTF_LINE_STRIP     0x0003
#define GLTF_TRIANGLES      0x0004
#define GLTF_TRIANGLE_STRIP 0x0005
#define GLTF_TRIANGLE_FAN   0x0006
#define GLTF_QUADS          0x0007
#define GLTF_QUAD_STRIP     0x0008

// Same as opengl types.. GL_INT, etc..
#define GLTF_ACCESSOR_COMPONENT_TYPE_INT 0x1404
#define GLTF_ACCESSOR_COMPONENT_TYPE_FLOAT 0x1406
#define GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_BYTE 0x1401

#define GLTF_ATTRIB_LOCATION_POSITION   0
#define GLTF_ATTRIB_LOCATION_NORMAL     1
#define GLTF_ATTRIB_LOCATION_TEX_COORD0 2
#define GLTF_ATTRIB_LOCATION_WEIGHTS0   3
#define GLTF_ATTRIB_LOCATION_JOINTS0    4


namespace platypus
{
    static std::string gltf_primitive_mode_to_string(int mode)
    {
        switch (mode)
        {
            case GLTF_POINTS :
                return "GLTF_POINTS";
            case GLTF_LINES :
                return "GLTF_LINES";
            case GLTF_LINE_LOOP :
                return "GLTF_LINE_LOOP";
            case GLTF_LINE_STRIP :
                return "GLTF_LINE_STRIP";
            case GLTF_TRIANGLES :
                return "GLTF_TRIANGLES";
            case GLTF_TRIANGLE_STRIP :
                return "GLTF_TRIANGLE_STRIP";
            case GLTF_TRIANGLE_FAN :
                return "GLTF_TRIANGLE_FAN";
            case GLTF_QUADS :
                return "GLTF_QUADS";
            case GLTF_QUAD_STRIP :
                return "GLTF_QUAD_STRIP";
            default:
                return "none";
        }
    }


    // TODO: get actual GLEnum values ftom specification to here!
    // Cannot use GLenums cuz if not using opengl api, these are not defined!
    static ShaderDataType gltf_accessor_component_type_to_engine(
        int gltfAccessorComponentType,
        int gltfComponentCount
    )
    {
        // float types
        if (gltfAccessorComponentType == GLTF_ACCESSOR_COMPONENT_TYPE_FLOAT)
        {
            switch (gltfComponentCount)
            {
                case (1) :
                    return ShaderDataType::Float;
                case (2) :
                    return ShaderDataType::Float2;
                case (3) :
                    return ShaderDataType::Float3;
                case (4) :
                    return ShaderDataType::Float4;
                default:
                    Debug::log(
                        "@gltf_accessor_component_type_to_engine(float) "
                        "invalid accessor component type: " + std::to_string(gltfAccessorComponentType) + " "
                        "Component count: " + std::to_string(gltfComponentCount),
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    return ShaderDataType::None;
            }
        }
        // int types
        else if (gltfAccessorComponentType == GLTF_ACCESSOR_COMPONENT_TYPE_INT)
        {
            switch (gltfComponentCount)
            {
                case (1) :
                    return ShaderDataType::Int;
                case (2) :
                    return ShaderDataType::Int2;
                case (3) :
                    return ShaderDataType::Int3;
                case (4) :
                    return ShaderDataType::Int4;
                default:
                    Debug::log(
                        "@gltf_accessor_component_type_to_engine(int) "
                        "invalid accessor component type: " + std::to_string(gltfAccessorComponentType),
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    return ShaderDataType::None;
            }
        }
        Debug::log(
            "@gltf_accessor_component_type_to_engine "
            "Unexpected input! "
            "gltf accessor component type: " + std::to_string(gltfAccessorComponentType) + " "
            "Component count:  " + std::to_string(gltfComponentCount),
            Debug::MessageType::PLATYPUS_ERROR
        );
        return ShaderDataType::None;
    }


    static Buffer* load_index_buffer(
        const CommandPool& commandPool,
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh,
        size_t primitiveIndex
    )
    {
        if (primitiveIndex >= gltfMesh.primitives.size())
        {
            Debug::log(
                "@load_index_buffer "
                "Invalid primitiveIndex: " + std::to_string(primitiveIndex) + " "
                "mesh primitive count: " + std::to_string(gltfMesh.primitives.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }

        const tinygltf::Primitive& primitive = gltfMesh.primitives[primitiveIndex];
        if (primitive.indices < 0 || primitive.indices >= gltfModel.accessors.size())
        {
            Debug::log(
                "@load_index_buffer "
                "Failed to find index buffer accessor using primitive.indices: " + std::to_string(primitive.indices),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }

        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
        size_t bufferLength = accessor.count;

        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
        const size_t elementSize = bufferView.byteLength / accessor.count;
        void* pData = gltfModel.buffers[bufferView.buffer].data.data() + bufferView.byteOffset + accessor.byteOffset;

        return new Buffer(
            commandPool,
            pData,
            elementSize,
            bufferLength,
            BUFFER_USAGE_INDEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_DST_BIT,
            BUFFER_UPDATE_FREQUENCY_STATIC
        );
    }


    struct GLTFVertexBufferAttrib
    {
        int location;
        ShaderDataType dataType;
        int bufferViewIndex;
    };
    // NOTE:
    // Current limitations!
    // * Expecting to have only a single set of primitives for single mesh!
    //  -> if thats not the case shit gets fucked!
    static Buffer* load_vertex_buffer(
        const CommandPool& commandPool,
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh
    )
    {
        size_t combinedVertexBufferSize = 0;
        size_t elementSize = 0;
        // sortedvertexbufferattributes is used to create single vertex buffer containing:
        //  * vertex positions
        //  * normals
        //  * uv coords
        //  * joint weights(if mesh has those)
        //  * joint indices(if mesh has those)
        // in that order starting from location = 0.
        //
        // key: attrib location
        // value:
        //  first: pretty obvious...
        //  second: index in gltfBufferViews
        std::unordered_map<int, GLTFVertexBufferAttrib> sortedVertexBufferAttributes;
        std::unordered_map<int, tinygltf::Accessor> attribAccessorMapping;
        std::unordered_map<int, size_t> actualAttribElemSize;
        for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
        {
            for (auto &attrib : gltfMesh.primitives[i].attributes)
            {
                const tinygltf::Accessor& accessor = gltfModel.accessors[attrib.second];

                int componentCount = 1;
                if (accessor.type != TINYGLTF_TYPE_SCALAR)
                    componentCount = accessor.type;

                // NOTE: using this expects shader having vertex attribs in same order
                // as here starting from pos 0
                int attribLocation = -1; // NOTE: attribLocation is more like attrib index here!
                if (attrib.first == "POSITION")     attribLocation = GLTF_ATTRIB_LOCATION_POSITION;
                if (attrib.first == "NORMAL")       attribLocation = GLTF_ATTRIB_LOCATION_NORMAL;
                if (attrib.first == "TEXCOORD_0")   attribLocation = GLTF_ATTRIB_LOCATION_TEX_COORD0;

                if (attrib.first == "WEIGHTS_0")    attribLocation = GLTF_ATTRIB_LOCATION_WEIGHTS0;
                if (attrib.first == "JOINTS_0")     attribLocation = GLTF_ATTRIB_LOCATION_JOINTS0;

                if (attribLocation > -1)
                {
                    // NOTE: Possible issue if file format expects some
                    // shit to be normalized on the application side..
                    int normalized = accessor.normalized;
                    if (normalized)
                    {
                        Debug::log(
                            "@load_vertex_buffer "
                            "Vertex attribute expected to be normalized from gltf file side "
                            "but engine doesnt currently support this!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        return nullptr;
                    }

                    // Joint indices may come as unsigned bytes -> switch those to float for now..
                    ShaderDataType shaderDataType = ShaderDataType::None;
                    if (accessor.componentType != GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_BYTE)
                    {
                        shaderDataType = gltf_accessor_component_type_to_engine(accessor.componentType, componentCount);
                    }
                    else
                    {
                        // NOTE: THIS SHOULD ONLY BE HAPPENING IF ATTRIB == JOINT
                        // OTHER CASES AREN'T HANDLED HERE!!!
                        if (attrib.first == "JOINTS_0")
                        {
                            shaderDataType = gltf_accessor_component_type_to_engine(
                                TINYGLTF_COMPONENT_TYPE_FLOAT,
                                componentCount
                            );
                            actualAttribElemSize[attribLocation] = 1 * 4; // 1 byte for each vec4 component
                        }
                        else
                        {
                            Debug::log(
                                "accessor.componentType was GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_BYTE "
                                "but the attrib isn't JOINTS_0! "
                                "Currently GLTF_ACCESSOR_COMPONENT_TYPE_UNSIGNED_BYTE are allowed only for "
                                "attributes of type: JOINTS_0",
                                Debug::MessageType::PLATYPUS_ERROR
                            );
                            return nullptr;
                        }
                    }
                    sortedVertexBufferAttributes[attribLocation] = { attribLocation, shaderDataType, accessor.bufferView };
                    attribAccessorMapping[attribLocation] = accessor;

                    size_t attribSize = get_shader_datatype_size(shaderDataType);
                    elementSize += attribSize;
                    // NOTE: Don't remember wtf this elemCount is...
                    size_t elemCount = accessor.count;
                    combinedVertexBufferSize += elemCount * attribSize;
                }
                else
                {
                    Debug::log(
                        "@load_vertex_buffer "
                        "Vertex attribute location was missing",
                        Debug::MessageType::PLATYPUS_WARNING
                    );
                }
            }
        }

        // Combine all into single raw buffer
        PE_byte* pCombinedRawBuffer = new PE_byte[combinedVertexBufferSize];
        const size_t attribCount = sortedVertexBufferAttributes.size();
        size_t srcOffsets[attribCount];
        memset(srcOffsets, 0, sizeof(size_t) * attribCount);
        size_t dstOffset = 0;

        size_t currentAttribIndex = 0;
        std::unordered_map<int, std::pair<Vector4f, Vector4f>> vertexJointData;
        int vertexIndex = 0;
        int i = 0;
        while (dstOffset < combinedVertexBufferSize)
        {
            const GLTFVertexBufferAttrib& currentAttrib = sortedVertexBufferAttributes[currentAttribIndex];
            size_t currentAttribElemSize = get_shader_datatype_size(currentAttrib.dataType);

            size_t gltfInternalSize = currentAttribElemSize;
            if (actualAttribElemSize.find(currentAttribIndex) != actualAttribElemSize.end())
                gltfInternalSize = actualAttribElemSize[currentAttribIndex];

            tinygltf::BufferView& bufView = gltfModel.bufferViews[currentAttrib.bufferViewIndex];
            PE_ubyte* pSrcBuffer = (PE_ubyte*)(gltfModel.buffers[bufView.buffer].data.data() + bufView.byteOffset + attribAccessorMapping[currentAttribIndex].byteOffset + srcOffsets[currentAttribIndex]);

            // If attrib "gltf internal type" wasn't float
            //  -> we need to convert it into that (atm done only for joint ids buf which are ubytes)
            if (currentAttrib.location == 4)
            {
                Vector4f val;
                val.x = (float)*pSrcBuffer;
                val.y = (float)*(pSrcBuffer + 1);
                val.z = (float)*(pSrcBuffer + 2);
                val.w = (float)*(pSrcBuffer + 3);
                vertexJointData[vertexIndex].second = val;

                memcpy(pCombinedRawBuffer + dstOffset, &val, sizeof(Vector4f));
            }
            else
            {
                // make all weights sum be 1
                if (currentAttrib.location == 3)
                {
                    Vector4f val;
                    val.x = (float)*pSrcBuffer;
                    val.y = (float)*(pSrcBuffer + sizeof(float));
                    val.z = (float)*(pSrcBuffer + sizeof(float) * 2);
                    val.w = (float)*(pSrcBuffer + sizeof(float) * 3);
                    const float sum = val.x + val.y + val.z + val.w;
                    if (sum != 0.0f)
                    {
                        val.x /= sum;
                        val.y /= sum;
                        val.z /= sum;
                        val.w /= sum;
                    }
                    else if (sum != 1.0f)
                    {
                        val = Vector4f(1.0f, 0, 0, 0);
                    }
                    vertexJointData[vertexIndex].first = val;

                    memcpy(pCombinedRawBuffer + dstOffset, &val, sizeof(Vector4f));
                }
                else
                {
                    memcpy(pCombinedRawBuffer + dstOffset, pSrcBuffer, gltfInternalSize);
                }
            }

            srcOffsets[currentAttribIndex] += gltfInternalSize;
            dstOffset += currentAttribElemSize;

            currentAttribIndex += 1;
            currentAttribIndex = currentAttribIndex % attribCount;

            ++i;
            if ((i % attribCount) == 0)
                ++vertexIndex;
        }
        size_t bufferLength = combinedVertexBufferSize / elementSize;
        Buffer* pBuffer = new Buffer(
            commandPool,
            pCombinedRawBuffer,
            elementSize,
            bufferLength,
            BUFFER_USAGE_VERTEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_DST_BIT,
            BUFFER_UPDATE_FREQUENCY_STATIC
        );
        delete[] pCombinedRawBuffer;
        return pBuffer;
    }


    // NOTE:
    // Current limitations:
    //  * single mesh only
    //      - also excluding any camera, light nodes, etc..
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
        std::vector<Buffer*>& outVertexBuffers,
        std::vector<Buffer*>& outIndexBuffers
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

        // TODO:
        //  * Support multiple meshes
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
        for (size_t i = 0; i < gltfModel.meshes.size(); ++i)
        {
            tinygltf::Mesh& gltfMesh = gltfModel.meshes[i];
            Buffer* pIndexBuffer = load_index_buffer(commandPool, gltfModel, gltfMesh, i);
            Buffer* pVertexBuffer = load_vertex_buffer(commandPool, gltfModel, gltfMesh);
            if (!pIndexBuffer)
            {
                Debug::log(
                    "@load_gltf_model "
                    "Failed to load index buffer for mesh: " + std::to_string(i) + " "
                    "from file: " + filepath,
                    Debug::MessageType::PLATYPUS_ERROR
                );
            }
            else if (!pVertexBuffer)
            {
                Debug::log(
                    "@load_gltf_model "
                    "Failed to load vertex buffer for mesh: " + std::to_string(i) + " "
                    "from file: " + filepath,
                    Debug::MessageType::PLATYPUS_ERROR
                );
            }
            outIndexBuffers.push_back(pIndexBuffer);
            outVertexBuffers.push_back(pVertexBuffer);
        }
        return true;
    }
}
