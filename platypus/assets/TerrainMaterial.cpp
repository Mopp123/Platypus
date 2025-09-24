#include "TerrainMaterial.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/PipelineFactory.hpp"
#include "AssetManager.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static std::vector<DescriptorSetLayoutBinding> create_descriptor_set_layout_bindings(uint32_t textureChannels)
    {
        int uniformLocationBegin = 8; // NOTE: Might be wrong atm for web
        std::vector<DescriptorSetLayoutBinding> layoutBindings;
        for (uint32_t i = 0; i < textureChannels + 1; ++i) // +1 since also the blendmap
        {
            layoutBindings.push_back(
                {
                    i,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { uniformLocationBegin + (int)i } }
                }
            );
        }
        return layoutBindings;
    }


    TerrainMaterial::TerrainMaterial(
        ID_t blendmapTextureID,
        ID_t* channelTextureIDs,
        size_t channelCount
    ) :
        Asset(AssetType::ASSET_TYPE_TERRAIN_MATERIAL),
        _blendmapTextureID(blendmapTextureID)
    {
        if (channelCount != PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@TerrainMaterial::TerrainMaterial " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS) + " required! "
                "Given channels: " + std::to_string(channelCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _descriptorSetLayout = { create_descriptor_set_layout_bindings(channelCount) };
        memcpy(_channelTextures, channelTextureIDs, sizeof(ID_t) * PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS);
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

        std::vector<DescriptorSetComponent> descriptorSetComponents;
        descriptorSetComponents.push_back({ DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, getBlendmapTexture() });
        for (uint32_t i = 0; i < PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS; ++i)
            descriptorSetComponents.push_back({ DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, getChannelTexture(i) });

        size_t maxFramesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
        for (size_t i = 0; i < maxFramesInFlight; ++i)
        {
            _descriptorSets.push_back(
                descriptorPool.createDescriptorSet(
                    &_descriptorSetLayout,
                    descriptorSetComponents
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

    Texture* TerrainMaterial::getBlendmapTexture() const
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _blendmapTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* TerrainMaterial::getChannelTexture(size_t channel) const
    {
        if (channel >= PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@TerrainMaterial::getChannelTexture "
                "Invalid channel: " + std::to_string(channel) + " "
                "Available channels: " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _channelTextures[channel],
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
