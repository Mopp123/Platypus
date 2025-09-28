#pragma once

#include "Asset.h"
#include "Texture.h"
#include "Mesh.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Pipeline.h"


namespace platypus
{
    struct MaterialPipelineData
    {
        Shader* pVertexShader;
        Shader* pFragmentShader;
        Pipeline* pPipeline;
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

        DescriptorSetLayout _descriptorSetLayout;

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
            const Mesh* pMesh
        );
        void createSkinnedPipeline(
            const Mesh* pMesh
        );

        void recreateExistingPipeline();
        void destroyPipeline();

        Texture* getDiffuseTexture() const;
        Texture* getSpecularTexture() const;
        Texture* getNormalTexture() const;
        std::vector<Texture*> getTextures() const;

        inline float getSpecularStrength() const { return _specularStrength; }
        inline float getShininess() const { return _shininess; }
        inline bool isShadeless() const { return _shadeless; }
        inline bool hasNormalMap() const { return _normalTextureID != NULL_ID;  }
        inline const MaterialPipelineData* getPipelineData() { return _pPipelineData; }
        inline const MaterialPipelineData* getSkinnedPipelineData() { return _pSkinnedPipelineData; }

        inline const DescriptorSetLayout& getDescriptorSetLayout() const { return _descriptorSetLayout; }

        inline const Shader* getVertexShader() const { return _pPipelineData->pVertexShader; }
        inline const Shader* getFragmentShader() const { return _pPipelineData->pFragmentShader; }

        inline const Shader* getSkinnedVertexShader() const { return _pSkinnedPipelineData->pVertexShader; }
        inline const Shader* getSkinnedFragmentShader() const { return _pSkinnedPipelineData->pFragmentShader; }

        void warnUnassigned(const std::string& beginStr);

    private:
        // Returns compiled shader filename depending on given properties
        std::string getShaderFilename(uint32_t shaderStage, bool normalMapping, bool skinned);
    };
}
