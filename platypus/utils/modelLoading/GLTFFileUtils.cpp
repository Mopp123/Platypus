#include "GLTFFileUtils.hpp"
#include <tiny_gltf.h>
#include "platypus/core/Debug.hpp"
#include <stdexcept>


namespace platypus
{
    std::string gltf_primitive_mode_to_string(int mode)
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
    ShaderDataType gltf_accessor_component_type_to_engine(
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
                    PLATYPUS_ASSERT(false);
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
                    PLATYPUS_ASSERT(false);
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
        PLATYPUS_ASSERT(false);
        return ShaderDataType::None;
    }


    std::string gltf_accessor_component_type_to_string(
        int gltfAccessorComponentType
    )
    {
        switch (gltfAccessorComponentType)
        {
            case TINYGLTF_COMPONENT_TYPE_BYTE: return "TINYGLTF_COMPONENT_TYPE_BYTE";
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return "TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE";
            case TINYGLTF_COMPONENT_TYPE_SHORT: return "TINYGLTF_COMPONENT_TYPE_SHORT";
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return "TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT";
            case TINYGLTF_COMPONENT_TYPE_INT: return "TINYGLTF_COMPONENT_TYPE_INT";
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return "TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT";
            case TINYGLTF_COMPONENT_TYPE_FLOAT: return "TINYGLTF_COMPONENT_TYPE_FLOAT";
            case TINYGLTF_COMPONENT_TYPE_DOUBLE: return "TINYGLTF_COMPONENT_TYPE_DOUBLE";
            default: return "ERROR";
        }
    }


    std::string gltf_accessor_type_to_string(
        int gltfAccessorType
    )
    {
        switch (gltfAccessorType)
        {
            case TINYGLTF_TYPE_SCALAR: return "TINYGLTF_TYPE_SCALAR";
            case TINYGLTF_TYPE_VEC2: return "TINYGLTF_TYPE_VEC2";
            case TINYGLTF_TYPE_VEC3: return "TINYGLTF_TYPE_VEC3";
            case TINYGLTF_TYPE_VEC4: return "TINYGLTF_TYPE_VEC4";
            case TINYGLTF_TYPE_MAT2: return "TINYGLTF_TYPE_MAT2";
            case TINYGLTF_TYPE_MAT3: return "TINYGLTF_TYPE_MAT3";
            case TINYGLTF_TYPE_MAT4: return "TINYGLTF_TYPE_MAT4";
            case TINYGLTF_TYPE_MATRIX: return "TINYGLTF_TYPE_MATRIX";
            default: return "ERROR";
        }
    }


    // Returns the attrib's indices like: JOINTS_1, WEIGHTS_2, etc as integers
    static int get_gltf_attrib_type_index(const std::string& gltfAttribType)
    {
        int index = 0;
        size_t numPos = gltfAttribType.find("_");
        if (numPos != std::string::npos)
        {
            if (numPos < gltfAttribType.size())
            {
                const size_t numLength = gltfAttribType.size() - numPos;
                std::string numStr = gltfAttribType.substr(numPos + 1, numLength);
                try
                {
                    index = std::stoi(numStr);
                }
                catch (const std::invalid_argument& e)
                {
                    Debug::log(
                        "Invalid string: " + gltfAttribType,
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }
                catch (const std::out_of_range& e)
                {
                    Debug::log(
                        "String too long(for signed integer): " + gltfAttribType,
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }
            }
        }
        return index;
    }


    VertexAttributeType gltf_attrib_type_to_engine(const std::string& gltfAttribType)
    {
        if (gltfAttribType == "POSITION")
        {
            return VertexAttributeType::POSITION;
        }
        else if (gltfAttribType == "NORMAL")
        {
            return VertexAttributeType::NORMAL;
        }
        else if (gltfAttribType == "TANGENT")
        {
            return VertexAttributeType::TANGENT;
        }
        else if (gltfAttribType.find("TEXCOORD_") != std::string::npos)
        {
            if (gltfAttribType != "TEXCOORD_0")
            {
                Debug::log(
                    "Invalid tex coord attribute: " + gltfAttribType + ". "
                    "Currently supporting only a single tex coord per vertex (TEXCOORD_0)",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return VertexAttributeType::CUSTOM;
            }
            return VertexAttributeType::TEX_COORD;
        }
        else if (gltfAttribType.find("COLOR_") != std::string::npos)
        {
            Debug::log(
                "Vertex attrib was: " + gltfAttribType + ". "
                "Currently not supporting vertex colors.",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return VertexAttributeType::CUSTOM;
        }
        else if (gltfAttribType.find("JOINTS_") != std::string::npos)
        {
            if (gltfAttribType != "JOINTS_0")
            {
                Debug::log(
                    "Currently supporting only 4 joint IDs per vertex, specified by JOINTS_0 attribute. "
                    "Given attribute was: " + gltfAttribType,
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return VertexAttributeType::CUSTOM;
            }
            return VertexAttributeType::JOINT;
        }
        else if (gltfAttribType.find("WEIGHTS_") != std::string::npos)
        {
            if (gltfAttribType != "WEIGHTS_0")
            {
                Debug::log(
                    "Currently supporting only 4 weights for 4 joint IDs per vertex, specified by WEIGHTS_0 attribute. "
                    "Given attribute was: " + gltfAttribType,
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return VertexAttributeType::CUSTOM;
            }
            return VertexAttributeType::JOINT;
        }

        Debug::log(
            "Invalid gltf vertex attribute type: " + gltfAttribType,
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return VertexAttributeType::CUSTOM;
    }


    Quaternion to_engine_quaternion(const std::vector<double>& gltfQuaternion)
    {
        if(gltfQuaternion.size() != 4)
        {
            Debug::log(
                "@to_engine_quaternion "
                "GLTF Quaternion had invalid component count: " + std::to_string(gltfQuaternion.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return { 0, 0, 0, 1 };
        }

        return Quaternion(
            (float)gltfQuaternion[0],
            (float)gltfQuaternion[1],
            (float)gltfQuaternion[2],
            (float)gltfQuaternion[3]
        );
    }


    Vector3f to_engine_vector3(const std::vector<double>& gltfVector)
    {
        if(gltfVector.size() != 3)
        {
            Debug::log(
                "@to_engine_vector3 "
                "GLTF Vector had invalid component count: " + std::to_string(gltfVector.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return { 0, 0, 0 };
        }

        return Vector3f(
            (float)gltfVector[0],
            (float)gltfVector[1],
            (float)gltfVector[2]
        );
    }


    Matrix4f to_engine_matrix(const std::vector<double>& gltfMatrix)
    {
        // Defaulting to 0 matrix to be able to test does this have matrix at all!
        Matrix4f matrix(0.0f);
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
}
