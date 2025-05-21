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
        // The material descriptor set layout, NOT including all common descriptor set layouts
        // used by the pipeline as well
        DescriptorSetLayout descriptorSetLayout;
        Shader _vertexShader;
        Shader _fragmentShader;
        Pipeline _pipeline;
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

        MaterialPipelineData _pipelineData;
        MaterialPipelineData _normalMappedPipelineData;

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

        void createPipeline();
        void createNormalMappedPipeline();

        Texture* getDiffuseTexture() const;
        Texture* getSpecularTexture() const;
        Texture* getNormalTexture() const;

        inline float getSpecularStrength() const { return _specularStrength; }
        inline float getShininess() const { return _shininess; }
        inline bool isShadeless() const { return _shadeless; }
        inline bool hasNormalMap() const { return _normalTextureID != NULL_ID;  }
    };
}
