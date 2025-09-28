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
        std::vector<DescriptorSetLayoutBinding> layoutBindings;
        const int diffuseChannelCount = PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS;
        const int specularChannelCount = PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS;
        const int totalTextureBindings = diffuseChannelCount + specularChannelCount + 1; // +1 since also the blendmap
        for (int i = 0; i < totalTextureBindings; ++i)
        {
            layoutBindings.push_back(
                {
                    (uint32_t)i,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { } }
                }
            );
        }

        layoutBindings.push_back(
            {
                (uint32_t)totalTextureBindings,
                1,
                DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                { { ShaderDataType::Float4 } }
            }
        );
        return layoutBindings;
    }


    TerrainMaterial::TerrainMaterial(
        ID_t blendmapTextureID,
        ID_t* diffuseChannelTextureIDs,
        size_t diffuseChannelCount,
        ID_t* specularChannelTextureIDs,
        size_t specularChannelCount
    ) :
        Asset(AssetType::ASSET_TYPE_TERRAIN_MATERIAL),
        _blendmapTextureID(blendmapTextureID)
    {
        if (diffuseChannelCount != PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@TerrainMaterial::TerrainMaterial " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS) + " "
                "required for diffuse channels! "
                "Given channels: " + std::to_string(diffuseChannelCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (specularChannelCount != PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@TerrainMaterial::TerrainMaterial " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS) + " "
                "required for specular channels! "
                "Given channels: " + std::to_string(specularChannelCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        _descriptorSetLayout = { create_descriptor_set_layout_bindings() };
        memcpy(
            _diffuseChannelTextureIDs,
            diffuseChannelTextureIDs,
            sizeof(ID_t) * PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS
        );
        memcpy(
            _specularChannelTextureIDs,
            specularChannelTextureIDs,
            sizeof(ID_t) * PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS
        );
    }

    TerrainMaterial::~TerrainMaterial()
    {
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

    Texture* TerrainMaterial::getBlendmapTexture() const
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _blendmapTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* TerrainMaterial::getDiffuseChannelTexture(size_t channel) const
    {
        if (channel >= PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@TerrainMaterial::getDiffuseChannelTexture "
                "Invalid channel: " + std::to_string(channel) + " "
                "Available channels: " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _diffuseChannelTextureIDs[channel],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* TerrainMaterial::getSpecularChannelTexture(size_t channel) const
    {
        if (channel >= PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@TerrainMaterial::getSpecularChannelTexture "
                "Invalid channel: " + std::to_string(channel) + " "
                "Available channels: " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _specularChannelTextureIDs[channel],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    std::vector<Texture*> TerrainMaterial::getTextures() const
    {
        const size_t totalChannelTextureCount = PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS * 2;
        const size_t totalTextureCount = totalChannelTextureCount + 1;
        std::vector<Texture*> textures(totalTextureCount);
        textures[0] = getBlendmapTexture();
        for (size_t i = 0; i < PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS; ++i)
        {
            size_t diffuseTargetIndex = 1 + i;
            size_t specularTargetIndex = 1 + i + PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS;
            textures[diffuseTargetIndex] = getDiffuseChannelTexture(i);
            textures[specularTargetIndex] = getSpecularChannelTexture(i);
        }

        return textures;
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
