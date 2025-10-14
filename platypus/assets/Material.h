#pragma once

#include "Asset.h"
#include "Texture.h"
#include "Mesh.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Pipeline.h"
#include <unordered_map>

#define PE_MAX_MATERIAL_TEX_CHANNELS 5


namespace platypus
{
    // Used to determine which shader to use
    enum class MaterialType
    {
        MESH,
        TERRAIN
    };

    struct MaterialPipelineData
    {
        Shader* pVertexShader;
        Shader* pFragmentShader;
        Pipeline* pPipeline;
        ~MaterialPipelineData()
        {
            delete pVertexShader;
            delete pFragmentShader;
            delete pPipeline;
        }
    };

    struct MaterialUniformBufferData
    {
        // x = specular strength, y = shininess, z = is shadeless, w = undefined for now
        Vector4f lightingProperties;
        // x,y = texture offset, z,w = texture scale
        Vector4f textureProperties;
    };

    class Material : public Asset
    {
    private:
        MaterialType _type;
        ID_t _blendmapTextureID = NULL_ID;
        ID_t _diffuseTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        ID_t _specularTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        ID_t _normalTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        size_t _diffuseTextureCount = 0;
        size_t _specularTextureCount = 0;
        size_t _normalTextureCount = 0;

        // TODO: Some map or something, containing all different pipelines instead of this mess...
        MaterialPipelineData* _pPipelineData = nullptr;
        MaterialPipelineData* _pShadowPipelineData = nullptr;
        MaterialPipelineData* _pSkinnedPipelineData = nullptr;
        MaterialPipelineData* _pSkinnedShadowPipelineData = nullptr;

        // TODO: Material instance
        MaterialUniformBufferData _uniformBufferData;

        DescriptorSetLayout _descriptorSetLayout;
        std::vector<Buffer*> _uniformBuffers;
        std::vector<DescriptorSet> _descriptorSets;

    public:
        Material(
            MaterialType type,
            ID_t blendmapTextureID,
            ID_t* pDiffuseTextureIDs,
            ID_t* pSpecularTextureIDs,
            ID_t* pNormalTextureIDs,
            size_t diffuseTextureCount,
            size_t specularTextureCount,
            size_t normalTextureCount,
            float specularStrength,
            float shininess,
            bool shadeless = false
        );
        ~Material();

        // TODO: Unfuck this mess plz!
        void createPipeline(
            const RenderPass* pRenderPass,
            const VertexBufferLayout& meshVertexBufferLayout,
            bool instanced,
            bool skinned,
            bool shadowPipeline
        );

        void recreateExistingPipeline();
        void destroyPipeline();

        void createShaderResources();
        void destroyShaderResources();

        Texture* getBlendmapTexture() const;
        Texture* getDiffuseTexture(size_t channel) const;
        Texture* getSpecularTexture(size_t channel) const;
        Texture* getNormalTexture(size_t channel) const;
        std::vector<Texture*> getTextures() const;

        void setLightingProperties(float specularStrength, float shininess, bool shadeless);
        void setTextureProperties(const Vector2f& textureOffset, const Vector2f& textureScale);

        // *Fuckin dumb, I know... just need to differentiate Asset type and Material type atm...
        inline MaterialType getMaterialType() const { return _type; }

        inline bool hasNormalMap() const { return _normalTextureIDs[0] != NULL_ID; }

        inline size_t getTotalTextureCount() const
        {
            return _diffuseTextureCount + _specularTextureCount + _normalTextureCount + (_blendmapTextureID != NULL_ID ? 1 : 0);
        }
        inline float getSpecularStrength() const { return _uniformBufferData.lightingProperties.x; }
        inline float getShininess() const { return _uniformBufferData.lightingProperties.y; }
        inline bool isShadeless() const { return _uniformBufferData.lightingProperties.z; }
        inline Vector2f getTextureOffset() const { return { _uniformBufferData.textureProperties.x, _uniformBufferData.textureProperties.y }; }
        inline Vector2f getTextureScale() const { return { _uniformBufferData.textureProperties.z, _uniformBufferData.textureProperties.w };; }

        inline const MaterialPipelineData* getPipelineData() { return _pPipelineData; }
        inline const MaterialPipelineData* getShadowPipelineData() { return _pShadowPipelineData; }
        inline const MaterialPipelineData* getSkinnedPipelineData() { return _pSkinnedPipelineData; }
        inline const MaterialPipelineData* getSkinnedShadowPipelineData() { return _pSkinnedShadowPipelineData; }

        inline const DescriptorSetLayout& getDescriptorSetLayout() const { return _descriptorSetLayout; }
        inline const std::vector<DescriptorSet> getDescriptorSets() const { return _descriptorSets; }

        inline const Shader* getVertexShader() const { return _pPipelineData->pVertexShader; }
        inline const Shader* getFragmentShader() const { return _pPipelineData->pFragmentShader; }

        inline const Shader* getSkinnedVertexShader() const { return _pSkinnedPipelineData->pVertexShader; }
        inline const Shader* getSkinnedFragmentShader() const { return _pSkinnedPipelineData->pFragmentShader; }

        void warnUnassigned(const std::string& beginStr);

    private:
        void validateTextureCounts();
        void createDescriptorSetLayout();
        // NOTE: This updates all uniform buffers for all possible frames in flight,
        // not sure should we be doing that here...
        void updateUniformBuffers();

        // Returns compiled shader filename depending on given properties
        std::string getShaderFilename(uint32_t shaderStage, bool skinned, bool shadow);
    };
}
