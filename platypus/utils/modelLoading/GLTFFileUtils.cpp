#include "GLTFFileUtils.h"
#include "platypus/core/Debug.h"


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
