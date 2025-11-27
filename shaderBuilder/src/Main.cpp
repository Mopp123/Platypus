#include "platypus/core/Debug.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Shader.h"

#include "ShaderBuilder.hpp"


using namespace platypus;
using namespace shaderBuilder;

int main(int argc, const char** argv)
{
    // Test inputs
    VertexBufferLayout vertexBufferLayout(
        {
            { 0, ShaderDataType::Float3 },
            { 1, ShaderDataType::Float4 }, // weights
            { 2, ShaderDataType::Float4 }, // jointIDs
            { 3, ShaderDataType::Float3 }, // normal
            { 4, ShaderDataType::Float2 } // tex coord
        },
        VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
        0
    );
    std::vector<std::string> vertexAttributeNames{
        "position",
        "normal",
        "weights",
        "jointIDs",
        "texCoord"
    };

    std::vector<UniformInfo> shadowPushConstants = {
        { ShaderDataType::Mat4 },
        { ShaderDataType::Mat4 }
    };
    std::vector<std::string> pushConstantsVariableNames{
        "projMat",
        "viewMat"
    };

    DescriptorSetLayoutBinding sceneDataDescriptorSetLayoutBinding{
        0,
        1,
        DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
        {
            { ShaderDataType::Mat4 },
            { ShaderDataType::Mat4 },
            { ShaderDataType::Float4 },
            { ShaderDataType::Float4 },
            { ShaderDataType::Float4 },
            { ShaderDataType::Float4 },
            { ShaderDataType::Float4 }
        }
    };
    std::vector<std::string> sceneDataDescriptorSetVariableNames{
        "projectionMatrix",
        "viewMatrix",
        "cameraPosition",
        "ambientLightColor",
        "lightDirection",
        "lightColor",
        "shadowProperties"
    };
    const size_t maxJoints = 32;
    DescriptorSetLayoutBinding jointDescriptorSetLayoutBinding{
        0,
        1,
        DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
        ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
        {
            { ShaderDataType::Mat4, (int)maxJoints },
        }
    };
    std::vector<std::string> jointDescriptorSetVariableNames{
        "data",
    };


    // Test vertex shader building
    ShaderStageBuilder vertexShaderBuilder(
        ShaderVersion::VULKAN_GLSL_450,
        ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
        { },
        0
    );
    vertexShaderBuilder.addVertexAttributes(
        {
            vertexBufferLayout
        },
        {
            vertexAttributeNames
        }
    );

    vertexShaderBuilder.addPushConstants(
        shadowPushConstants,
        pushConstantsVariableNames,
        "shadowPushConstants"
    );

    vertexShaderBuilder.addDescriptorSet(
        { sceneDataDescriptorSetLayoutBinding },
        {
            sceneDataDescriptorSetVariableNames,
        },
        "SceneData",
        "sceneData"
    );

    vertexShaderBuilder.addDescriptorSet(
        { jointDescriptorSetLayoutBinding },
        { jointDescriptorSetVariableNames },
        "JointData",
        "jointData"
    );

    vertexShaderBuilder.build();

    // Test fragment shader building
    const size_t textureBindingCount = 1;
    DescriptorSetLayoutBinding materialTextureBinding{
        0,
        1,
        DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
        { { } }
    };

    DescriptorSetLayoutBinding materialDataBinding{
        textureBindingCount,
        1,
        DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
        {
            { ShaderDataType::Float4 }, // x = specular strength, y = shininess, z = is shadeless, w = undefined for now
            { ShaderDataType::Float4 } // x,y = texture offset, z,w = texture scale
        }
    };

    ShaderStageBuilder fragmentShaderBuilder(
        ShaderVersion::VULKAN_GLSL_450,
        ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
        vertexShaderBuilder.getOutput(),
        vertexShaderBuilder.getDescriptorSetCount()
    );
    fragmentShaderBuilder.addMaterial(
        { },
        { materialTextureBinding },
        { materialTextureBinding, materialTextureBinding, materialTextureBinding },
        { materialTextureBinding, materialTextureBinding },
        materialDataBinding
    );

    fragmentShaderBuilder.build();

    Debug::log("Built shader: ");
    std::string source;
    for (const std::string& line : fragmentShaderBuilder.getLines())
        source += line;

    Debug::log(source);

    return 0;
}
