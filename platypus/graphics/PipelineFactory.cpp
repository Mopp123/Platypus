#include "PipelineFactory.hpp"
#include "platypus/core/Application.h"
#include "Swapchain.h"
#include "renderers/MasterRenderer.h"
#include <string>


namespace platypus
{

    static std::string get_shader_filename(uint32_t shaderStage, bool normalMapping, bool skinned)
    {
        // Example shader names:
        // vertex shader: "StaticVertexShader", "StaticHDVertexShader", "SkinnedHDVertexShader"
        // fragment shader: "StaticFragmentShader", "StaticHDFragmentShader", "SkinnedHDFragmentShader"
        std::string shaderName = "";
        if (!skinned)
            shaderName += "Static";
        else
            shaderName += "Skinned";

        if (normalMapping)
            shaderName += "HD";

        if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
            shaderName += "Vertex";
        else if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
            shaderName += "Fragment";

        shaderName += "Shader";

        return shaderName;
    }

    Pipeline* create_material_pipeline(
        const Mesh* pMesh,
        bool instanced,
        bool skinned,
        Material* pMaterial
    )
    {
        // Figure out vertex buffer layouts
        const VertexBufferLayout& meshVBLayout = pMesh->getVertexBufferLayout();
        std::vector<VertexBufferLayout> useVertexBufferLayouts = { meshVBLayout };
        if (instanced)
        {
            uint32_t meshVBLayoutElements = (uint32_t)meshVBLayout.getElements().size();
            VertexBufferLayout instancedVBLayout = {
                {
                    { meshVBLayoutElements, ShaderDataType::Float4 },
                    { meshVBLayoutElements + 1, ShaderDataType::Float4 },
                    { meshVBLayoutElements + 2, ShaderDataType::Float4 },
                    { meshVBLayoutElements + 3, ShaderDataType::Float4 }
                },
                VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
                1
            };
            useVertexBufferLayouts.push_back(instancedVBLayout);
        }

        // Figure out descriptor set layouts and shaders
        const Shader* pUseVertexShader = nullptr;
        const Shader* pUseFragmentShader = nullptr;
        const MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        std::vector<DescriptorSetLayout> useDescriptorSetLayouts = {
            pMasterRenderer->getCameraDescriptorSetLayout(),
            pMasterRenderer->getDirectionalLightDescriptorSetLayout()
        };
        if (skinned)
        {
            // NOTE: Does this descriptor set layout really need to live in the SkinnedMeshRenderer?
            // TODO: Rather just make it here?
            useDescriptorSetLayouts.push_back(
                pMasterRenderer->getSkinnedMeshRenderer()->getDescriptorSetLayout()
            );

            pUseVertexShader = pMaterial->getSkinnedVertexShader();
            pUseFragmentShader = pMaterial->getSkinnedFragmentShader();
        }
        else
        {
            pUseVertexShader = pMaterial->getVertexShader();
            pUseFragmentShader = pMaterial->getFragmentShader();
        }

        useDescriptorSetLayouts.push_back(pMaterial->getDescriptorSetLayout());

        const Swapchain& swapchain = Application::get_instance()->getMasterRenderer()->getSwapchain();
        // NOTE: Currently sending proj mat as push constant
        Pipeline* pPipeline = new Pipeline(
            swapchain.getRenderPassPtr(),
            useVertexBufferLayouts,
            useDescriptorSetLayouts,
            pUseVertexShader,
            pUseFragmentShader,
            CullMode::CULL_MODE_BACK,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            true, // Enable color blend
            sizeof(Matrix4f), // Push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // Push constants' stage flags
        );

        return pPipeline;
    }
}
