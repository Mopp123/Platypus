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
    struct MaterialPipelineData
    {
        Shader* pVertexShader;
        Shader* pFragmentShader;
        Pipeline* pPipeline;
        ~MaterialPipelineData()
        {
            // NOTE: IMPORTANT to destroy pipeline before shaders since
            // web impl detaches the shaders from the opengl shader program
            // when destroying the pipeline!
            delete pPipeline;
            delete pVertexShader;
            delete pFragmentShader;
        }
    };

    struct MaterialUniformBufferData
    {
        // x = specular strength,
        // y = shininess,
        // z = is shadeless,
        // w = unused atm
        Vector4f lightingProperties;
        // x,y = texture offset, z,w = texture scale
        Vector4f textureProperties;
    };

    class Material : public Asset
    {
    private:
        // NOTE: Do these really need to be like this?
        ID_t _blendmapTextureID = NULL_ID;
        ID_t _diffuseTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        ID_t _specularTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        ID_t _normalTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        size_t _diffuseTextureCount = 0;
        size_t _specularTextureCount = 0;
        size_t _normalTextureCount = 0;

        bool _castShadows = false;
        bool _receiveShadows = false;
        uint32_t _shadowmapDescriptorIndex = 0;

        std::unordered_map<MeshType, MaterialPipelineData*> _pipelines;

        // TODO: Material instance
        MaterialUniformBufferData _uniformBufferData;

        DescriptorSetLayout _descriptorSetLayout;
        std::vector<Buffer*> _uniformBuffers;
        std::vector<DescriptorSet> _descriptorSets;

        std::string _customVertexShaderFilename;
        std::string _customFragmentShaderFilename;

    public:
        Material(
            ID_t blendmapTextureID,
            ID_t* pDiffuseTextureIDs,
            ID_t* pSpecularTextureIDs,
            ID_t* pNormalTextureIDs,
            size_t diffuseTextureCount,
            size_t specularTextureCount,
            size_t normalTextureCount,
            float specularStrength,
            float shininess,
            const Vector2f& textureOffset = { 0, 0 },
            const Vector2f& textureScale = { 1, 1 },
            bool castShadows = false,
            bool receiveShadows = false,
            const std::string& customVertexShaderFilename = "",
            const std::string& customFragmentShaderFilename = ""
        );
        ~Material();

        void createPipeline(
            const RenderPass* pRenderPass,
            MeshType meshType
        );

        void recreateExistingPipeline();
        void destroyPipeline();

        void createShaderResources();
        void destroyShaderResources();

        void updateShadowmapDescriptorSet(Texture* pShadowmapTexture);

        Texture* getBlendmapTexture() const;
        Texture* getDiffuseTexture(size_t channel) const;
        Texture* getSpecularTexture(size_t channel) const;
        Texture* getNormalTexture(size_t channel) const;
        std::vector<Texture*> getTextures() const;

        void setLightingProperties(float specularStrength, float shininess, bool shadeless);
        void setTextureProperties(const Vector2f& textureOffset, const Vector2f& textureScale);

        Pipeline* getPipeline(MeshType meshType);

        inline bool hasBlendmap() const { return _blendmapTextureID != NULL_ID; }
        inline bool hasNormalMap() const { return _normalTextureIDs[0] != NULL_ID; }

        inline size_t getTotalTextureCount() const
        {
            return _diffuseTextureCount +
                _specularTextureCount +
                _normalTextureCount +
                (_blendmapTextureID != NULL_ID ? 1 : 0) +
                (_receiveShadows ? 1 : 0);
        }
        inline float getSpecularStrength() const { return _uniformBufferData.lightingProperties.x; }
        inline float getShininess() const { return _uniformBufferData.lightingProperties.y; }
        inline bool castsShadows() const { return _castShadows; }
        inline bool receivesShadows() const { return _receiveShadows; }
        inline bool isShadeless() const { return _uniformBufferData.lightingProperties.z; }
        inline Vector2f getTextureOffset() const { return { _uniformBufferData.textureProperties.x, _uniformBufferData.textureProperties.y }; }
        inline Vector2f getTextureScale() const { return { _uniformBufferData.textureProperties.z, _uniformBufferData.textureProperties.w };; }

        inline const DescriptorSetLayout& getDescriptorSetLayout() const { return _descriptorSetLayout; }
        inline const std::vector<DescriptorSet> getDescriptorSets() const { return _descriptorSets; }

        void warnUnassigned(const std::string& beginStr);

    private:
        void validateTextureCounts();
        void createDescriptorSetLayout();
        // NOTE: This updates all uniform buffers for all possible frames in flight,
        // not sure should we be doing that here...
        void updateUniformBuffers(size_t frame);

        // Returns compiled shader filename depending on given properties
        std::string getShaderFilename(uint32_t shaderStage, MeshType meshType);
    };
}
