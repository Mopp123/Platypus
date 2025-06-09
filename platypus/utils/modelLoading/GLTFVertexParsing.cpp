#include "GLTFVertexParsing.h"
#include "platypus/core/Debug.h"
#include "GLTFFileUtils.h"

#include <tiny_gltf.h>
#include <cstring>
#include <utility>


namespace platypus
{
    // Sorts attributes in a way that the indices gravitate towards "preferred locations"
    static bool sort_gltf_attributes(
        const std::string gltfAttribName,
        std::vector<std::string>& outAttribs
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

        int preferredLocation = preferredLocations[gltfAttribName];
        bool ready = false;
        for (int i = 0; i < (int)outAttribs.size(); ++i)
        {
            if (ready)
                return true;

            const std::string foundAttribName = outAttribs[i];
            int foundAttribPreferredLocation = preferredLocations[foundAttribName];
            if (preferredLocation < foundAttribPreferredLocation)
            {
                outAttribs[i] = gltfAttribName;
                if (foundAttribName != "none")
                    ready = sort_gltf_attributes(foundAttribName, outAttribs);
                else
                    break;
            }
        }
        return true;
    }

    static void solve_gltf_attribs(
        tinygltf::Mesh& gltfMesh,
        std::unordered_map<std::string, int>& outAttribLocationMapping
    )
    {
        const size_t maxAttribs = 6;
        std::vector<std::string> sortedAttribNames(maxAttribs);
        for (size_t i = 0; i < maxAttribs; ++i)
            sortedAttribNames[i] = "none";

        for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
        {
            for (auto &attrib : gltfMesh.primitives[i].attributes)
            {
                sort_gltf_attributes(attrib.first, sortedAttribNames);
            }
        }
        for (size_t i = 0; i < sortedAttribNames.size(); ++i)
        {
            const std::string& attribName = sortedAttribNames[i];
            if (attribName == "none")
                break;

            outAttribLocationMapping[attribName] = i;
        }
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
        // key = location
        std::map<uint32_t, ShaderDataType> orderedBufferElements;
        size_t combinedVertexBufferSize = 0;
        size_t elementSize = 0;

        // sortedvertexbufferattributes is used to create single vertex buffer
        //
        // key: attrib location
        // value:
        //  first: pretty obvious...
        //  second: index in gltfBufferViews
        std::unordered_map<int, GLTFVertexBufferAttrib> sortedVertexBufferAttributes;
        std::unordered_map<std::string, int> gltfAttribNameToLocation;

        solve_gltf_attribs(
            gltfMesh,
            gltfAttribNameToLocation
        );
        std::unordered_map<std::string, int>::iterator testIt;

        Debug::log("___TEST___SOLVED ATTRIBS");
        for (testIt = gltfAttribNameToLocation.begin(); testIt != gltfAttribNameToLocation.end(); ++testIt)
        {
            Debug::log("    " + testIt->first + " location: " + std::to_string(testIt->second));
        }


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
                int attribLocation = gltfAttribNameToLocation[attrib.first];

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
                            //actualAttribElemSize[attribLocation] = 1 * 4; // 1 byte for each vec4 component
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

                    Debug::log(
                        "___TEST___loaded attrib: \n"
                        "   location: " + std::to_string(attribLocation) + "\n"
                        "   data type: " + std::to_string(shaderDataType) + "\n"
                        "   attrib size: " + std::to_string(attribSize) + "\n"
                        "   accessor count: " + std::to_string(elemCount)
                    );

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

        Debug::log("___TEST___START COMBINING VERTEX BUFFER! Using size: " + std::to_string(combinedVertexBufferSize));
        // NOTE: SOMETHING FUCKS UP HERE IF LOADING SKINNED MODEL
        // Attribs go 0, 1, 2, to 0 again...  doesnt include skinning attribs?

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
            // NOTE; FOUND *THE* ISSUE HERE!!
            // If model not having tangents -> can't access below at currentAttribIndex
            // since index 3 isn't defined if having skinning but no tangents!
            const GLTFVertexBufferAttrib& currentAttrib = sortedVertexBufferAttributes[currentAttribIndex];
            size_t currentAttribElemSize = get_shader_datatype_size(currentAttrib.dataType);
            Debug::log(
                "___TEST___using dst offset: " + std::to_string(dstOffset) +
                " buffer size: " + std::to_string(combinedVertexBufferSize) + " "
                "current attrib size: " + std::to_string(currentAttribElemSize)
            );

            size_t gltfInternalSize = currentAttribElemSize;
            //if (actualAttribElemSize.find(currentAttribIndex) != actualAttribElemSize.end())
            //    gltfInternalSize = actualAttribElemSize[currentAttribIndex];

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


                if (dstOffset + sizeof(Vector4f) > combinedVertexBufferSize)
                    PLATYPUS_ASSERT(false);

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

                    if (dstOffset + sizeof(Vector4f) > combinedVertexBufferSize)
                        PLATYPUS_ASSERT(false);

                    memcpy(combinedRawBuffer.data() + dstOffset, &val, sizeof(Vector4f));
                }
                else
                {
                    if (dstOffset + gltfInternalSize > combinedVertexBufferSize)
                    {
                        Debug::log(
                            "Writing out of bounds! "
                            "range: " + std::to_string(dstOffset) + " to"
                            " " + std::to_string(dstOffset + gltfInternalSize) + " "
                            "attrib location: " + std::to_string(currentAttrib.location) + " "
                            "attrib size: " + std::to_string(gltfInternalSize),
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                    }

                    memcpy(combinedRawBuffer.data() + dstOffset, pSrcBuffer, gltfInternalSize);
                }
            }

            srcOffsets[currentAttribIndex] += gltfInternalSize;
            dstOffset += currentAttribElemSize;
            Debug::log("___TEST___DST OFFSET ADVANCED TO: " + std::to_string(dstOffset));

            currentAttribIndex += 1;
            currentAttribIndex = currentAttribIndex % attribCount;

            ++i;
            if ((i % attribCount) == 0)
                ++vertexIndex;
        }
        size_t bufferLength = combinedVertexBufferSize / elementSize;
        Debug::log("___TEST___VERTEX BUFFER COMBINED! size = " + std::to_string(combinedVertexBufferSize));
        Debug::log("___TEST___VERTEX BUFFER ELEM SIZE = " + std::to_string(elementSize));
        Debug::log("___TEST___VERTEX BUFFER ATTRIB COUNT = " + std::to_string(attribCount));
        outBufferData = { elementSize, bufferLength, combinedRawBuffer };

        Debug::log("___TEST___SORTING VERTEX BUFFER ELEMENTS...");
        std::vector<VertexBufferElement> sortedVertexBufferElements;
        for (const auto& elem : orderedBufferElements)
            sortedVertexBufferElements.push_back({ elem.first, elem.second});

        Debug::log("___TEST___CREATING VERTEX BUFFER LAYOUT");
        outLayout = { sortedVertexBufferElements, VertexInputRate::VERTEX_INPUT_RATE_VERTEX, 0 };
        Debug::log("___TEST___CREATED VERTEX BUFFER LAYOUT SUCCESSFULLY");
        return true;
    }
}
