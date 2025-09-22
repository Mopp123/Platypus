#include "TerrainMaterial.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/PipelineFactory.hpp"
#include "AssetManager.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static std::vector<DescriptorSetLayoutBinding> create_descriptor_set_layout_bindings()
    {
        return {
            {
                0,
                1,
                DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                { { 5 } } // NOTE: Might be wrong atm for web
            }
        };
    }


    TerrainMaterial::TerrainMaterial(
        ID_t diffuseTextureID
    ) :
        Asset(AssetType::ASSET_TYPE_TERRAIN_MATERIAL),
        _diffuseTextureID(diffuseTextureID)
    {
        _descriptorSetLayout = { create_descriptor_set_layout_bindings() };
    }

    TerrainMaterial::~TerrainMaterial()
    {
        freeShaderResources();

        _descriptorSetLayout.destroy();
        // TODO: Unfuck below
        // NOTE: Important that the pipeline gets destroyed before shaders,
        // since the web implementation detaches the shaders from opengl
        // shader program on pipeline destruction.
        //  -> if shaders destroyed before pipeline, the detaching in
        //  OpenglShaderProgram breaks
        if (_pPipelineData)
        {
            delete _pPipelineData->pPipeline;
            delete _pPipelineData->pVertexShader;
            delete _pPipelineData->pFragmentShader;
            delete _pPipelineData;
        }
    }

    void TerrainMaterial::createPipeline()
    {
        if (_pPipelineData != nullptr)
        {
            Debug::log(
                "@TerrainMaterial::createPipeline "
                "Pipeline data already exists for material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        const std::string vertexShaderFilename = "TerrainVertexShader";
        const std::string fragmentShaderFilename = "TerrainFragmentShader";
        Debug::log(
            "@TerrainMaterial::createPipeline Using shaders:\n    " + vertexShaderFilename + "\n    " + fragmentShaderFilename
        );

        // TODO: Unfuck below
        _pPipelineData = new TerrainMaterialPipelineData;
        Shader* pVertexShader = new Shader(
            vertexShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        Shader* pFragmentShader = new Shader(
            fragmentShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );
        _pPipelineData->pVertexShader = pVertexShader;
        _pPipelineData->pFragmentShader = pFragmentShader;
        Pipeline* pPipeline = create_terrain_material_pipeline(this);
        _pPipelineData->pPipeline = pPipeline;
        _pPipelineData->pPipeline->create();
    }

    void TerrainMaterial::recreateExistingPipeline()
    {
        bool created = false;

        if (_pPipelineData)
        {
            _pPipelineData->pPipeline->create();
            created = true;
        }

        if (!created)
            warnUnassigned("@TerrainMaterial::recreateExistingPipeline");
    }

    void TerrainMaterial::destroyPipeline()
    {
        bool destroyed = false;
        if (_pPipelineData)
        {
            _pPipelineData->pPipeline->destroy();
            destroyed = true;
            Debug::log("@TerrainMaterial::destroyPipeline Static pipeline destroyed");
        }

        if (!destroyed)
            warnUnassigned("@TerrainMaterial::destroyPipeline");
    }

    void TerrainMaterial::createShaderResources()
    {
        if (!_pPipelineData)
        {
            Debug::log(
                "@TerrainMaterial::createDescriptorSets "
                "Pipeline data was nullptr for terrain material with ID: " + std::to_string(getID()) + " "
                "This material may have been created but never used by any renderable component!",
                Debug::MessageType::PLATYPUS_WARNING
            );
            return;
        }

        if (!_descriptorSets.empty())
        {
            Debug::log(
                "@TerrainMaterial::createDescriptorSets "
                "Descriptor sets already exists for terrain material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        DescriptorPool& descriptorPool = pMasterRenderer->getDescriptorPool();

        const Texture* pDiffuseTexture = getDiffuseTexture();

        size_t maxFramesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
        for (size_t i = 0; i < maxFramesInFlight; ++i)
        {
            _descriptorSets.push_back(
                descriptorPool.createDescriptorSet(
                    &_descriptorSetLayout,
                    {
                        { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pDiffuseTexture }
                    }
                )
            );
        }

        Debug::log(
            "@TerrainMaterial::createDescriptorSets "
            "New descriptor sets created for terrain material: " + std::to_string(getID())
        );
    }

    void TerrainMaterial::freeShaderResources()
    {
        if (!_pPipelineData)
        {
            Debug::log(
                "@TerrainMaterial::freeDescriptorSets "
                "Pipeline data was nullptr for terrain material with ID: " + std::to_string(getID()) + " "
                "This material may have been created but never used by any renderable component!",
                Debug::MessageType::PLATYPUS_WARNING
            );
            return;
        }

        DescriptorPool& descriptorPool = Application::get_instance()->getMasterRenderer()->getDescriptorPool();
        descriptorPool.freeDescriptorSets(_descriptorSets);
        _descriptorSets.clear();
    }

    Texture* TerrainMaterial::getDiffuseTexture() const
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _diffuseTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    void TerrainMaterial::warnUnassigned(const std::string& beginStr)
    {
        Debug::log(
            beginStr +
            " Pipeline data was nullptr. "
            "This TerrainMaterial may have been created but not used by any renderable component.",
            Debug::MessageType::PLATYPUS_WARNING
        );
    }
}
