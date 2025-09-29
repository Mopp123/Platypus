#include "PipelineFactory.hpp"
#include "platypus/core/Application.h"
#include "Swapchain.h"
#include "platypus/assets/TerrainMesh.hpp"
#include "renderers/MasterRenderer.h"
#include "renderers/Batch.hpp"
#include <string>


namespace platypus
{
    Pipeline* create_material_pipeline(
        const VertexBufferLayout& meshVertexBufferLayout,
        bool instanced,
        bool skinned,
        Material* pMaterial
    )
    {
        // Figure out vertex buffer layouts
        std::vector<VertexBufferLayout> useVertexBufferLayouts = { meshVertexBufferLayout };
        if (instanced)
        {
            uint32_t meshVBLayoutElements = (uint32_t)meshVertexBufferLayout.getElements().size();
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
            pMasterRenderer->getScene3DDataDescriptorSetLayout(),
        };
        if (skinned)
        {
            // NOTE: Too confusing to access joint descriptor set layout from Batcher here?!
            // TODO: Fix plz?
            useDescriptorSetLayouts.push_back(
                Batcher::get_joint_descriptor_set_layout()
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
            0, // Push constants size
            ShaderStageFlagBits::SHADER_STAGE_NONE // Push constants' stage flags
        );

        return pPipeline;
    }

    Pipeline* create_terrain_material_pipeline(
        const VertexBufferLayout& meshVertexBufferLayout,
        Material* pMaterial
    )
    {
        // Figure out descriptor set layouts and shaders
        const MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        std::vector<DescriptorSetLayout> useDescriptorSetLayouts = {
            pMasterRenderer->getScene3DDataDescriptorSetLayout(),
            Batcher::get_terrain_descriptor_set_layout(),
            pMaterial->getDescriptorSetLayout()
        };

        const Swapchain& swapchain = Application::get_instance()->getMasterRenderer()->getSwapchain();
        // NOTE: Currently sending proj mat as push constant
        Pipeline* pPipeline = new Pipeline(
            swapchain.getRenderPassPtr(),
            { meshVertexBufferLayout },
            useDescriptorSetLayouts,
            pMaterial->getVertexShader(),
            pMaterial->getFragmentShader(),
            CullMode::CULL_MODE_NONE,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            true, // Enable color blend
            0, // Push constants size
            ShaderStageFlagBits::SHADER_STAGE_NONE // Push constants' stage flags
        );

        return pPipeline;
    }
}
