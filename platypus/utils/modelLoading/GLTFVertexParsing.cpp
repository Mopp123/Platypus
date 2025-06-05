#include "GLTFVertexParsing.h"
#include "platypus/core/Debug.h"
#include "GLTFFileUtils.h"

#include <tiny_gltf.h>


namespace platypus
{
    bool load_index_buffers(
        const CommandPool& commandPool,
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh,
        std::vector<MeshBufferData>& outBufferData
    )
    {
        for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
        {
            const tinygltf::Primitive& primitive = gltfMesh.primitives[i];
            if (primitive.indices < 0 || primitive.indices >= gltfModel.accessors.size())
            {
                Debug::log(
                    "@load_index_buffer "
                    "Failed to find index buffer accessor using primitive.indices: " + std::to_string(primitive.indices),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return false;
            }

            const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
            size_t bufferLength = accessor.count;

            const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const size_t elementSize = bufferView.byteLength / accessor.count;
            size_t bufferSize = elementSize * bufferLength;

            std::vector<PE_byte> rawBuffer(bufferSize);
            memcpy(
                rawBuffer.data(),
                gltfModel.buffers[bufferView.buffer].data.data() + bufferView.byteOffset + accessor.byteOffset,
                bufferSize
            );

            outBufferData.push_back({ elementSize, bufferLength, rawBuffer });
        }
        return true;
    }


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
    )
    {
        // key = location
        std::map<uint32_t, ShaderDataType> orderedBufferElements;
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
                if (attrib.first == "TANGENT")      attribLocation = GLTF_ATTRIB_LOCATION_TANGENT;

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
                        return false;
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
                            return false;
                        }
                    }
                    sortedVertexBufferAttributes[attribLocation] = { attribLocation, shaderDataType, accessor.bufferView };
                    attribAccessorMapping[attribLocation] = accessor;

                    size_t attribSize = get_shader_datatype_size(shaderDataType);
                    elementSize += attribSize;
                    // NOTE: Don't remember wtf this elemCount is...
                    size_t elemCount = accessor.count;
                    combinedVertexBufferSize += elemCount * attribSize;

                    orderedBufferElements[(uint32_t)attribLocation] = shaderDataType;
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
        std::vector<PE_byte> combinedRawBuffer(combinedVertexBufferSize);
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

                memcpy(combinedRawBuffer.data() + dstOffset, &val, sizeof(Vector4f));
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

                    memcpy(combinedRawBuffer.data() + dstOffset, &val, sizeof(Vector4f));
                }
                else
                {
                    memcpy(combinedRawBuffer.data() + dstOffset, pSrcBuffer, gltfInternalSize);
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
        outBufferData = { elementSize, bufferLength, combinedRawBuffer };

        std::vector<VertexBufferElement> sortedVertexBufferElements;
        for (const auto& elem : orderedBufferElements)
            sortedVertexBufferElements.push_back({ elem.first, elem.second});

        outLayout = { sortedVertexBufferElements, VertexInputRate::VERTEX_INPUT_RATE_VERTEX, 0 };
        return true;
    }
}
