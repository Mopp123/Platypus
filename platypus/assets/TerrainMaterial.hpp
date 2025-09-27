#pragma once

#include "Asset.h"
#include "Texture.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Pipeline.h"

#define PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS 5

namespace platypus
{
    struct TerrainMaterialPipelineData
    {
        Shader* pVertexShader;
        Shader* pFragmentShader;
        Pipeline* pPipeline;
    };

    class TerrainMaterial : public Asset
    {
    private:
        ID_t _blendmapTextureID = NULL_ID;
        ID_t _diffuseChannelTextureIDs[PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS];
        ID_t _specularChannelTextureIDs[PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS];

        float _specularStrength = 1.0f;
        float _shininess = 1.0f;
        bool _shadeless = false;

        TerrainMaterialPipelineData* _pPipelineData = nullptr;
        // NOTE: This contains ALL descriptor set layouts for pipeline!
        // Currently the last one is the actual material's layout!
        DescriptorSetLayout _descriptorSetLayout;

    public:
        TerrainMaterial(
            ID_t blendmapTextureID,
            ID_t* diffuseChannelTextureIDs,
            size_t diffuseChannelCount,
            ID_t* specularChannelTextureIDs,
            size_t specularChannelCount
        );
        ~TerrainMaterial();

        void createPipeline();

        void recreateExistingPipeline();
        void destroyPipeline();

        Texture* getBlendmapTexture() const;
        Texture* getDiffuseChannelTexture(size_t channel) const;
        Texture* getSpecularChannelTexture(size_t channel) const;
        std::vector<Texture*> getTextures() const;

        inline const TerrainMaterialPipelineData* getPipelineData() { return _pPipelineData; }

        inline const DescriptorSetLayout& getDescriptorSetLayout() const { return _descriptorSetLayout; }

        inline const Shader* getVertexShader() const { return _pPipelineData->pVertexShader; }
        inline const Shader* getFragmentShader() const { return _pPipelineData->pFragmentShader; }

        void warnUnassigned(const std::string& beginStr);

    private:
        // TODO: Need to implement similarly as regular Material, when having
        // different shaders for "high and low detail" versions
        //std::string getShaderFilename(uint32_t shaderStage);
    };
}
