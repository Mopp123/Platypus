#include "platypus/core/Debug.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Shader.h"

#include "ShaderBuilder.hpp"


using namespace platypus;

int main(int argc, const char** argv)
{
    // Test inputs
    VertexBufferLayout vertexBufferLayout(
        {
            { 0, ShaderDataType::Float3 },
            { 1, ShaderDataType::Float3 },
            { 2, ShaderDataType::Float2 },
            { 4, ShaderDataType::Float4 }
        },
        VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
        0
    );
    VertexBufferLayout instancedVertexBufferLayout(
        {
            { 0, ShaderDataType::Mat4 }
        },
        VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
        0
    );
    std::vector<std::string> vertexAttributeNames{
        "position",
        "normal",
        "texCoord",
        "tangent"
    };
    std::vector<std::string> instancedVertexAttributeNames{
        "transformationMatrix",
    };

    std::vector<UniformInfo> shadowPushConstants = {
        { ShaderDataType::Mat4 },
        { ShaderDataType::Mat4 }
    };
    std::vector<std::string> pushConstantsVariableNames{
        "projMat",
        "viewMat"
    };

    DescriptorSetLayoutBinding descriptorSetLayoutBinding{
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
    std::vector<std::string> descriptorSetVariableNames{
        "projectionMatrix",
        "viewMatrix",
        "cameraPosition",
        "ambientLightColor",
        "lightDirection",
        "lightColor",
        "shadowProperties"
    };


    // Test shader building
    shaderBuilder::VulkanShaderStageBuilder vertexShaderBuilder;
    vertexShaderBuilder.addVersion(ShaderVersion::VULKAN_GLSL_450);
    vertexShaderBuilder.addVertexAttributes(
        {
            vertexBufferLayout,
            instancedVertexBufferLayout
        },
        {
            vertexAttributeNames,
            instancedVertexAttributeNames
        }
    );

    vertexShaderBuilder.addPushConstants(
        shadowPushConstants,
        pushConstantsVariableNames,
        "shadowPushConstants"
    );

    vertexShaderBuilder.addUniformBlock(
        descriptorSetLayoutBinding,
        descriptorSetVariableNames,
        "SceneData",
        "sceneData"
    );

    vertexShaderBuilder.beginFunction(
        "main",
        ShaderDataType::None,
        { }
    );
    {
        // TODO: Something like below
        //ShaderVariable worldSpaceVertexPosition{ShaderDataType::Float4, ""};
        //vertexShaderBuilder.calcVertexPosition();

        vertexShaderBuilder.instantiateObject(
            "Vertex",
            "vertex",
            {
                { ShaderDataType::Float3, "position" },
                { ShaderDataType::Float3, "normal" },
                { ShaderDataType::Float2, "texCoord" },
                { ShaderDataType::Float4, "tangent" }
            }
        );

        vertexShaderBuilder.calcVertexWorldPosition();
        vertexShaderBuilder.printVariables();

        vertexShaderBuilder.setOutputTEST(0, ShaderDataType::Float3, "var_fragCoord");
        vertexShaderBuilder.setOutputTEST(2, ShaderDataType::Float2, "var_texCoord");
        vertexShaderBuilder.setOutputTEST(3, ShaderDataType::Float4, "var_TEST");
    }
    vertexShaderBuilder.endFunction({});

    Debug::log("Built shader: ");
    std::string source;
    for (const std::string& line : vertexShaderBuilder.getLines())
        source += line;

    Debug::log(source);

    return 0;
}
