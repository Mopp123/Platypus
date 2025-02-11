#include "ModelLoading.h"
#include "platypus/core/Debug.h"
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


namespace pk
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
                        Debug::MessageType::PK_FATAL_ERROR
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
                        Debug::MessageType::PK_FATAL_ERROR
                    );
                    return ShaderDataType::None;
            }
        }
        Debug::log(
            "@gltf_accessor_component_type_to_engine "
            "Unexpected input! "
            "gltf accessor component type: " + std::to_string(gltfAccessorComponentType) + " "
            "Component count:  " + std::to_string(gltfComponentCount),
            Debug::MessageType::PK_FATAL_ERROR
        );
        return ShaderDataType::None;
    }


    static Buffer* load_index_buffer(tinygltf::Model& gltfModel, tinygltf::Mesh& gltfMesh, size_t primitiveIndex)
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

        tinygltf::Primitive primitive = gltfmesh.primitives[primitiveIndex];
        if (primitive.indices < 0 || primitive.indices >= gltfModel.accessors.size())
        {
            Debug::log(
                "@load_index_buffer "
                "Failed to find index buffer accessor using primitive.indices: " + std::to_string(primitive.indices),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }

        const tinygltf::Accessor& accessor = &gltfModel.accessors[primitive.indices];
        size_t bufferLength = accessor.count;

        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferview];
        const size_t elementSize = bufferView.byteLength / accessor.count;
        void* pData = gltfModel.buffers[bufferView.buffer].data.data() + bufferView.byteOffset + accessor.byteOffset;

        return new Buffer(
            const CommandPool& commandPool,
            pData,
            elementSize,
            bufferLength,
            BUFFER_USAGE_INDEX_BUFFER_BIT | BUFFER_USAGE_TRANSFER_DST_BIT,
            BUFFER_UPDATE_FREQUENCY_STATIC
        );
    }


    // NOTE:
    // Current limitations!
    // * Expecting to have only a single set of primitives for single mesh!
    //  -> if thats not the case shit gets fucked!
    static Buffer* load_mesh_gltf(
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh
    )
    {
        size_t combinedvertexbuffersize = 0;

        // sortedvertexbufferattributes is used to create single vertex buffer containing
        // vertex positions, normals and uv coords in that order starting from location = 0.
        // this is also for creating vertexbufferlayout in correct order.
        //
        // key: attrib location
        // value:
        //  first: pretty obvious...
        //  second: index in gltfbuffers
        std::unordered_map<int, std::pair<vertexbufferelement, int>> sortedvertexbufferattributes;
        std::unordered_map<int, tinygltf::accessor> attribaccessormapping;
        std::unordered_map<int, size_t> actualattribelemsize;
        for (size_t i = 0; i < gltfmesh.primitives.size(); ++i)
        {
            tinygltf::primitive primitive = gltfmesh.primitives[i];

            if (!pindexbufferaccessor)
            {
                pindexbufferaccessor = &gltfmodel.accessors[primitive.indices];
                indexbufferlength = pindexbufferaccessor->count;
            }

            if (indexbufferlength <= 0)
            {
                debug::log(
                    "@load_mesh_gltf "
                    "index buffer length was 0",
                    debug::messagetype::pk_fatal_error
                );
                return nullptr;
            }

            for (auto &attrib : primitive.attributes)
            {
                tinygltf::accessor accessor = gltfmodel.accessors[attrib.second];

                int componentcount = 1;
                if (accessor.type != tinygltf_type_scalar)
                    componentcount = accessor.type;

                // note: using this expects shader having vertex attribs in same order
                // as here starting from pos 0
                int attriblocation = -1; // note: attriblocation is more like attrib index here!
                if (attrib.first == "position")     attriblocation = gltf_attrib_location_position;
                if (attrib.first == "normal")       attriblocation = gltf_attrib_location_normal;
                if (attrib.first == "texcoord_0")   attriblocation = gltf_attrib_location_tex_coord0;

                if (attrib.first == "weights_0")    attriblocation = gltf_attrib_location_weights0;
                if (attrib.first == "joints_0")     attriblocation = gltf_attrib_location_joints0;

                if (attriblocation > -1)
                {
                    // note: possible issue if file format expects some
                    // shit to be normalized on the application side..
                    int normalized = accessor.normalized;
                    if (normalized)
                    {
                        debug::log(
                            "@load_mesh_gltf "
                            "vertex attribute expected to be normalized from gltf file side "
                            "but engine doesnt currently support this!",
                            debug::messagetype::pk_fatal_error
                        );
                    }

                    // joint indices may come as unsigned bytes -> switch those to float for now..
                    shaderdatatype shaderdatatype = shaderdatatype::none;
                    if (accessor.componenttype != gltf_accessor_component_type_unsigned_byte)
                    {
                        shaderdatatype = gltf_accessor_component_type_to_engine(accessor.componenttype, componentcount);
                    }
                    else
                    {
                        shaderdatatype = gltf_accessor_component_type_to_engine(
                            tinygltf_component_type_float,
                            componentcount
                        );
                        actualattribelemsize[attriblocation] = 1 * 4; // 1 byte for each vec4 component
                    }
                    size_t attribsize = get_shader_data_type_size(shaderdatatype);

                    int bufferviewindex = accessor.bufferview;
                    vertexbufferelement elem(attriblocation, shaderdatatype);
                    sortedvertexbufferattributes[attriblocation] = std::make_pair(elem, bufferviewindex);
                    attribaccessormapping[attriblocation] = accessor;

                    size_t elemcount = accessor.count;
                    combinedvertexbuffersize += elemcount * attribsize;
                }
                else
                {
                    debug::log(
                        "@load_mesh_gltf "
                        "vertex attribute location was missing",
                        debug::messagetype::pk_warning
                    );
                }
            }
        }

        if (!pindexbufferaccessor)
        {
            debug::log(
                "@load_mesh_gltf "
                "failed to find index buffer from gltf model! currently index buffer is required for all meshes!",
                debug::messagetype::pk_fatal_error
            );
            return nullptr;
        }
        const tinygltf::bufferview& indexbufferview = gltfmodel.bufferviews[pindexbufferaccessor->bufferview];
        const size_t indexbufelemsize = indexbufferview.bytelength / pindexbufferaccessor->count;
        void* pindexbufferdata = gltfmodel.buffers[indexbufferview.buffer].data.data() + indexbufferview.byteoffset + pindexbufferaccessor->byteoffset;
        buffer* pindexbuffer = buffer::create(
            pindexbufferdata,
            indexbufelemsize,
            indexbufferlength,
            bufferusageflagbits::buffer_usage_index_buffer_bit,
            bufferupdatefrequency::buffer_update_frequency_static,
            false
        );

        pk_byte* pcombinedrawbuffer = new pk_byte[combinedvertexbuffersize];
        const size_t attribcount = sortedvertexbufferattributes.size();
        size_t srcoffsets[attribcount];
        memset(srcoffsets, 0, sizeof(size_t) * attribcount);
        size_t dstoffset = 0;
        std::vector<vertexbufferelement> vblayoutelements(sortedvertexbufferattributes.size());

        size_t currentattribindex = 0;


        // testing
        const size_t stride = sizeof(float) * 3 * 2 + sizeof(float) * 2 + sizeof(float) * 4 * 2;
        const size_t vertexcount = combinedvertexbuffersize / stride;
        std::unordered_map<int, std::pair<vec4, vec4>> vertexjointdata;
        int vertexindex = 0;
        int i = 0;

        while (dstoffset < combinedvertexbuffersize)
        {
            const std::pair<vertexbufferelement, int>& currentattrib = sortedvertexbufferattributes[currentattribindex];
            size_t currentattribelemsize = get_shader_data_type_size(currentattrib.first.gettype());

            size_t gltfinternalsize = currentattribelemsize;
            if (actualattribelemsize.find(currentattribindex) != actualattribelemsize.end())
                gltfinternalsize = actualattribelemsize[currentattribindex];

            tinygltf::bufferview& bufview = gltfmodel.bufferviews[currentattrib.second];
            pk_ubyte* psrcbuffer = (pk_ubyte*)(gltfmodel.buffers[bufview.buffer].data.data() + bufview.byteoffset + attribaccessormapping[currentattribindex].byteoffset + srcoffsets[currentattribindex]);

            // if attrib "gltf internal type" wasn't float
            //  -> we need to convert it into that (atm done only for joint ids buf which are ubytes)
            if (currentattrib.first.getlocation() == gltf_attrib_location_joints0)
            {
                vec4 val;
                val.x = (float)*psrcbuffer;
                val.y = (float)*(psrcbuffer + 1);
                val.z = (float)*(psrcbuffer + 2);
                val.w = (float)*(psrcbuffer + 3);
                vertexjointdata[vertexindex].second = val;

                memcpy(pcombinedrawbuffer + dstoffset, &val, sizeof(vec4));
            }
            else
            {
                // make all weights sum be 1
                if (currentattrib.first.getlocation() == gltf_attrib_location_weights0)
                {
                    vec4 val;
                    val.x = (float)*psrcbuffer;
                    val.y = (float)*(psrcbuffer + sizeof(float));
                    val.z = (float)*(psrcbuffer + sizeof(float) * 2);
                    val.w = (float)*(psrcbuffer + sizeof(float) * 3);
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
                        val = vec4(1.0f, 0, 0, 0);
                    }
                    vertexjointdata[vertexindex].first = val;

                    memcpy(pcombinedrawbuffer + dstoffset, &val, sizeof(vec4));
                }
                else
                {
                    memcpy(pcombinedrawbuffer + dstoffset, psrcbuffer, gltfinternalsize);
                }
            }


            srcoffsets[currentattribindex] += gltfinternalsize;
            dstoffset += currentattribelemsize;
            // danger!!!!
            vblayoutelements[currentattrib.first.getlocation()] = currentattrib.first; // danger!!!

            currentattribindex += 1;
            currentattribindex = currentattribindex % attribcount;

            ++i;
            if ((i % 5) == 0)
                ++vertexindex;
        }

        buffer* pvertexbuffer = buffer::create(
            pcombinedrawbuffer,
            1,
            combinedvertexbuffersize,
            bufferusageflagbits::buffer_usage_vertex_buffer_bit,
            bufferupdatefrequency::buffer_update_frequency_static,
            false
        );

        debug::log(
            "@load_mesh_gltf creating mesh with vertex buffer layout:"
        );
        for (const vertexbufferelement& e : vblayoutelements)
            debug::log("\tlocation = " + std::to_string(e.getlocation()) + " type = " + std::to_string(e.gettype()));

        pmesh = new mesh(
            { pvertexbuffer },
            pindexbuffer,
            nullptr
        );
        return pmesh;
    }


    static mat4 to_engine_matrix(const std::vector<double>& gltfMatrix)
    {
        // Defaulting to 0 matrix to be able to test does this have matrix at all!
        mat4 matrix(0.0f);
        if (gltfMatrix.size() == 16)
        {
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                    matrix[i + j * 4] = gltfMatrix[i + j * 4];
            }
        }
        return matrix;
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
        std::vector<Buffer*> meshes;
        for (size_t i = 0; i < gltfModel.meshes.size(); ++i)
        {
            tinygltf::Mesh& gltfMesh = gltfModel.meshes[i];
            Mesh* pMesh = load_mesh_gltf(
                gltfModel,
                gltfMesh,
                true
            );
            if (pMesh)
                meshes.push_back(pMesh);
        }

        // Load skeleton if found
        size_t skinsCount = gltfModel.skins.size();
        Pose bindPose;
        std::vector<Pose> animPoses;
        if (skinsCount == 1)
        {
            int rootJointNodeIndex = gltfModel.skins[0].joints[0];
            // Mapping from gltf joint node index to our pose struct's joint index
            std::unordered_map<int, int> nodeJointMapping;
            add_joint(
                gltfModel,
                bindPose,
                -1, // index to pose struct's parent joint. NOT glTF node index!
                rootJointNodeIndex,
                nodeJointMapping
            );
            Debug::log("___TEST___loaded joints: " + std::to_string(nodeJointMapping.size()));


            // Load animations if found
            // NOTE: For now supporting just a single animation
            size_t animCount = gltfModel.animations.size();
            if (animCount > 1)
            {
                Debug::log(
                    "@load_model_gltf "
                    "Multiple animations(" + std::to_string(animCount) + ") "
                    "from file: " + filepath + " Currently only a single animation is supported",
                    Debug::MessageType::PK_FATAL_ERROR
                );
            }
            if (animCount == 1)
            {
                animPoses = load_anim_poses(
                    gltfModel,
                    bindPose,
                    nodeJointMapping
                );
            }

            // Load inverse bind matrices
            const tinygltf::Accessor& invBindAccess = gltfModel.accessors[gltfModel.skins[0].inverseBindMatrices];
            const tinygltf::BufferView& invBindBufView = gltfModel.bufferViews[invBindAccess.bufferView];
            const tinygltf::Buffer& invBindBuf = gltfModel.buffers[invBindBufView.buffer];
            size_t offset = invBindBufView.byteOffset + invBindAccess.byteOffset;
            for (int i = 0; i < bindPose.joints.size(); ++i)
            {
                mat4 inverseBindMatrix(1.0f);
                memcpy(&inverseBindMatrix, invBindBuf.data.data() + offset, sizeof(mat4));
                bindPose.joints[i].inverseMatrix = inverseBindMatrix;
                offset += sizeof(float) * 16;
            }
            // NOTE: Temporarely assign mesh's bind and animation poses here
            // TODO: Load bind pose an anim poses in same func where mesh loading happens!
            // TODO: Support multiple meshes
            meshes[0]->setBindPose(bindPose);
            meshes[0]->setAnimationPoses(animPoses);
        }
        else if (skinsCount > 1)
        {
            Debug::log(
                "@load_model_gltf "
                "Too many skins in file: " + std::to_string(skinsCount) + " "
                "Currently only 1 is supported",
                Debug::MessageType::PK_FATAL_ERROR
            );
            return nullptr;
        }


        if (skinsCount == 1)
        {
            return new Model(meshes);
        }
        else
            return new Model(meshes);
    }
}
