#pragma once

#include "Asset.h"
#include "Texture.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Pipeline.h"


namespace platypus
{
    struct MaterialPipelineData
    {
        std::vector<VertexBufferLayout> vertexBufferLayouts;
        Shader vertexShader;
        Shader fragmentShader;
        Pipeline pipeline;
    };

    class Material : public Asset
    {
    private:
        ID_t _diffuseTextureID = NULL_ID;
        ID_t _specularTextureID = NULL_ID;
        ID_t _normalTextureID = NULL_ID;

        float _specularStrength = 1.0f;
        float _shininess = 1.0f;

        bool _shadeless = false;

        MaterialPipelineData* _pPipelineData = nullptr;
        MaterialPipelineData* _pSkinnedPipelineData = nullptr;
        std::vector<Buffer*> _uniformBuffers;
        // NOTE: This contains ALL descriptor set layouts for pipeline!
        // Currently the last one is the actual material's layout!
        DescriptorSetLayout _descriptorSetLayout;
        DescriptorSetLayout _skinnedDescriptorSetLayout;
        std::vector<DescriptorSet> _descriptorSets;

    public:
        Material(
            ID_t diffuseTextureID,
            ID_t specularTextureID,
            ID_t normalTextureID,
            float specularStrength = 1.0f,
            float shininess = 1.0f,
            bool shadeless = false
        );
        ~Material();

        void createPipeline(
            const VertexBufferLayout& meshVertexBufferLayout
        );
        void createSkinnedPipeline(
            const VertexBufferLayout& meshVertexBufferLayout
        );

        void recreateExistingPipeline();
        void destroyPipeline();
        // NOTE: This also creates the uniform buffer
        //  -> should that be a separate func or name this more clearly?
        void createDescriptorSets();
        void freeDescriptorSets();

        Texture* getDiffuseTexture() const;
        Texture* getSpecularTexture() const;
        Texture* getNormalTexture() const;

        inline float getSpecularStrength() const { return _specularStrength; }
        inline float getShininess() const { return _shininess; }
        inline bool isShadeless() const { return _shadeless; }
        inline bool hasNormalMap() const { return _normalTextureID != NULL_ID;  }
        inline const MaterialPipelineData* getPipelineData() { return _pPipelineData; }
        inline const MaterialPipelineData* getSkinnedPipelineData() { return _pSkinnedPipelineData; }
        inline const std::vector<Buffer*>& getUniformBuffers() const { return _uniformBuffers; }
        inline const std::vector<DescriptorSet>& getDescriptorSets() const { return _descriptorSets; }
        inline bool hasDescriptorSets() const { return !_descriptorSets.empty(); }

        void warnUnassigned(const std::string& beginStr);

    private:
        // Returns compiled shader filename depending on given properties
        std::string getShaderFilename(uint32_t shaderStage, bool normalMapping, bool skinned);
    };
}
