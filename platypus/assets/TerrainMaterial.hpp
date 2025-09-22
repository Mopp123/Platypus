#pragma once

#include "Asset.h"
#include "Texture.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Pipeline.h"


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
        ID_t _diffuseTextureID = NULL_ID;

        TerrainMaterialPipelineData* _pPipelineData = nullptr;
        // NOTE: This contains ALL descriptor set layouts for pipeline!
        // Currently the last one is the actual material's layout!
        DescriptorSetLayout _descriptorSetLayout;
        std::vector<DescriptorSet> _descriptorSets;

    public:
        TerrainMaterial(
            ID_t diffuseTextureID
        );
        ~TerrainMaterial();

        void createPipeline();

        void recreateExistingPipeline();
        void destroyPipeline();
        // NOTE: This also creates the uniform buffer
        //  -> should that be a separate func or name this more clearly?
        void createShaderResources();
        void freeShaderResources();

        Texture* getDiffuseTexture() const;

        inline const TerrainMaterialPipelineData* getPipelineData() { return _pPipelineData; }
        inline const std::vector<DescriptorSet>& getDescriptorSets() const { return _descriptorSets; }
        inline bool hasDescriptorSets() const { return !_descriptorSets.empty(); }

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
