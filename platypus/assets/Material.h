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
        // NOTE: This contains ALL descriptor set layouts for pipeline!
        // Currently the last one is the actual material's layout!
        std::vector<DescriptorSetLayout> descriptorSetLayouts;
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
        std::vector<Buffer*> _uniformBuffers;
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
            const VertexBufferLayout& meshVertexBufferLayout,
            bool skinned
        );
        void recreateExistingPipeline();
        void destroyPipeline();
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
        inline const std::vector<Buffer*>& getUniformBuffers() const { return _uniformBuffers; }
        inline const std::vector<DescriptorSet>& getDescriptorSets() const { return _descriptorSets; }

    private:
        std::string getVertexShaderFilename(uint32_t shaderStage, bool normalMapping);
    };
}
