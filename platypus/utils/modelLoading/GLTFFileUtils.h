#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/utils/Maths.h"
#include <vector>
#include <string>


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
#define GLTF_ATTRIB_LOCATION_TANGENT    3
#define GLTF_ATTRIB_LOCATION_WEIGHTS0   4
#define GLTF_ATTRIB_LOCATION_JOINTS0    5


namespace platypus
{
    struct GLTFVertexBufferAttrib
    {
        std::string name;
        int location;
        ShaderDataType dataType;
        int bufferViewIndex;
    };


    std::string gltf_primitive_mode_to_string(int mode);

    // TODO: get actual GLEnum values ftom specification to here!
    // Cannot use GLenums -> if not using opengl api, these are not defined!
    ShaderDataType gltf_accessor_component_type_to_engine(
        int gltfAccessorComponentType,
        int gltfComponentCount
    );

    Quaternion to_engine_quaternion(const std::vector<double>& gltfQuaternion);
    Vector3f to_engine_vector3(const std::vector<double>& gltfVector);
    Matrix4f to_engine_matrix(const std::vector<double>& gltfMatrix);
}
