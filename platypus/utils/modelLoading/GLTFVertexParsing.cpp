#include "GLTFVertexParsing.h"
#include "platypus/core/Debug.h"
#include "GLTFFileUtils.h"

#include <tiny_gltf.h>
#include <cstring>
#include <utility>


namespace platypus
{

    static GLTFVertexBufferAttrib find_attrib(
        const std::string& name,
        const std::vector<GLTFVertexBufferAttrib>& attribs
    )
    {
        for (const GLTFVertexBufferAttrib& attrib : attribs)
        {
            if (attrib.name == name)
                return attrib;
        }
        return { };
    }


    // Adds to outAttribs in a way that the indices gravitate towards "preferred locations"
    static bool add_to_sorted_attributes(
        GLTFVertexBufferAttrib attrib,
        std::vector<GLTFVertexBufferAttrib>& outAttribs
    )
    {
        // Preferred location for inputted attrib
        std::unordered_map<std::string, int> preferredLocations =
        {
            { "POSITION",   0 },
            { "NORMAL",     1 },
            { "TEXCOORD_0", 2 },
            { "TANGENT",    3 },
            { "WEIGHTS_0",  4 },
            { "JOINTS_0",   5 },
            { "none",   666 }
        };

        int preferredLocation = preferredLocations[attrib.name];
        bool ready = false;
        for (int i = 0; i < (int)outAttribs.size(); ++i)
        {
            if (ready)
                return true;

            GLTFVertexBufferAttrib foundAttrib = find_attrib(outAttribs[i].name, outAttribs);
            int foundAttribPreferredLocation = preferredLocations[foundAttrib.name];
            if (preferredLocation < foundAttribPreferredLocation)
            {
                attrib.location = i;
                outAttribs[i] = attrib;
                if (foundAttrib.name != "none")
                    ready = add_to_sorted_attributes(foundAttrib, outAttribs);
                else
                    break;
            }
        }
        return true;
    }


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
        if (gltfMesh.primitives.size() != 1)
        {
            Debug::log(
                "@load_vertex_buffer(gltf mesh) "
                "glTF mesh had " + std::to_string(gltfMesh.primitives.size()) + " primitives. "
                "Currently a single primitive object per mesh is required!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        size_t combinedVertexBufferSize = 0;
        size_t elementSize = 0;

        const tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[0];
        size_t attributeCount = gltfPrimitive.attributes.size();
        std::vector<GLTFVertexBufferAttrib> sortedVertexBufferAttributes(attributeCount);
        for (GLTFVertexBufferAttrib& attrib : sortedVertexBufferAttributes)
            attrib.name = "none";

        std::unordered_map<int, tinygltf::Accessor> attribAccessorMapping;
        for (auto &attrib : gltfPrimitive.attributes)
        {
            const tinygltf::Accessor& accessor = gltfModel.accessors[attrib.second];

            int componentCount = 1;
            if (accessor.type != TINYGLTF_TYPE_SCALAR)
                componentCount = accessor.type;

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

            size_t attribSize = get_shader_datatype_size(shaderDataType);
            elementSize += attribSize;
            // NOTE: Don't remember wtf this elemCount is...
            size_t elemCount = accessor.count;
            combinedVertexBufferSize += elemCount * attribSize;

            GLTFVertexBufferAttrib vertexBufferAttrib = {
                attrib.first,
                0,
                shaderDataType,
                accessor.bufferView,
                accessor.byteOffset
            };
            add_to_sorted_attributes(vertexBufferAttrib, sortedVertexBufferAttributes);
            // testing
            size_t byteStride = gltfModel.bufferViews[accessor.bufferView].byteStride;
            if (attrib.first == "WEIGHTS_0")
            {
                Debug::log("___TEST___WEIGHTS TYPE: " + gltf_accessor_component_type_to_string(accessor.componentType) + " COMPONENTS: " + std::to_string(componentCount));
            }
            else
            if (attrib.first == "JOINTS_0")
            {
                Debug::log("___TEST___JOINTS TYPE: " + gltf_accessor_component_type_to_string(accessor.componentType) + " COMPONENTS: " + std::to_string(componentCount));
            }
        }

        // Make sure all attributes were handled
        std::vector<size_t> nonePositions;
        for (size_t i = 0; i < sortedVertexBufferAttributes.size(); ++i)
        {
            if (sortedVertexBufferAttributes[i].name == "none")
            {
                Debug::log(
                    "@load_vertex_buffer(gltf mesh) "
                    "Attribute at location " + std::to_string(i) + " was none.",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }

        // Combine all into single raw buffer
        std::vector<PE_byte> combinedRawBuffer(combinedVertexBufferSize);
        const size_t attribCount = sortedVertexBufferAttributes.size();
        std::vector<size_t> srcOffsets(attribCount);
        memset(srcOffsets.data(), 0, sizeof(size_t) * attribCount);
        size_t dstOffset = 0;

        size_t currentAttribIndex = 0;
        std::unordered_map<int, std::pair<Vector4f, Vector4f>> vertexJointData;
        int vertexIndex = 0;
        int i = 0;
        while (dstOffset < combinedVertexBufferSize)
        {
            const GLTFVertexBufferAttrib& currentAttrib = sortedVertexBufferAttributes[currentAttribIndex];
            size_t currentAttribElemSize = get_shader_datatype_size(currentAttrib.dataType);
            // This is the size in the gltf buffer
            size_t internalAttribElemSize = currentAttribElemSize;
            if (dstOffset + currentAttribElemSize > combinedVertexBufferSize)
            {
                Debug::log(
                    "Writing out of bounds! "
                    "range: " + std::to_string(dstOffset) + " to"
                    " " + std::to_string(dstOffset + currentAttribElemSize) + " "
                    "attrib location: " + std::to_string(currentAttrib.location) + " "
                    "attrib size: " + std::to_string(currentAttribElemSize),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            tinygltf::BufferView& bufView = gltfModel.bufferViews[currentAttrib.bufferViewIndex];
            PE_ubyte* pSrcBuffer = (PE_ubyte*)(gltfModel.buffers[bufView.buffer].data.data() + bufView.byteOffset + currentAttrib.accessorByteOffset + srcOffsets[currentAttribIndex]);

            // If attrib "gltf internal type" wasn't float
            //  -> we need to convert it into that (atm done only for joint ids buf which are ubytes)
            if (currentAttrib.name == "JOINTS_0")
            {
                Vector4f val;
                val.x = (float)*pSrcBuffer;
                val.y = (float)*(pSrcBuffer + 1);
                val.z = (float)*(pSrcBuffer + 2);
                val.w = (float)*(pSrcBuffer + 3);
                vertexJointData[vertexIndex].second = val;

                //Debug::log("___TEST___JOINT IDs: " + val.toString() + " ATTRIB SIZE: " + std::to_string(currentAttribElemSize));

                if (dstOffset + sizeof(Vector4f) > combinedVertexBufferSize)
                    PLATYPUS_ASSERT(false);

                memcpy(combinedRawBuffer.data() + dstOffset, &val, sizeof(Vector4f));

                internalAttribElemSize = 4;
            }
            else
            {
                // make all weights sum be 1
                if (currentAttrib.name == "WEIGHTS_0")
                {
                    Vector4f val;
                    //val.x = (float)*pSrcBuffer;
                    //val.y = (float)*(pSrcBuffer + sizeof(float));
                    //val.z = (float)*(pSrcBuffer + sizeof(float) * 2);
                    //val.w = (float)*(pSrcBuffer + sizeof(float) * 3);
                    //val = val.normalize();
                    memcpy((void*)(&val), pSrcBuffer, sizeof(Vector4f));
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
                    Debug::log("___TEST___WEIGHTS: " + val.toString() + " ATTRIB SIZE: " + std::to_string(currentAttribElemSize));
                    vertexJointData[vertexIndex].first = val;

                    if (dstOffset + sizeof(Vector4f) > combinedVertexBufferSize)
                        PLATYPUS_ASSERT(false);

                    memcpy(combinedRawBuffer.data() + dstOffset, &val, sizeof(Vector4f));
                }
                else
                {
                    memcpy(combinedRawBuffer.data() + dstOffset, pSrcBuffer, currentAttribElemSize);
                }
            }

            srcOffsets[currentAttribIndex] += internalAttribElemSize;
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
        for (const GLTFVertexBufferAttrib& attrib : sortedVertexBufferAttributes)
            sortedVertexBufferElements.push_back({ (uint32_t)attrib.location, attrib.dataType});

        outLayout = { sortedVertexBufferElements, VertexInputRate::VERTEX_INPUT_RATE_VERTEX, 0 };
        return true;
    }
}
