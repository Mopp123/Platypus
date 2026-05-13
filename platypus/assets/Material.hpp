#pragma once

#include "Asset.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"
#include "platypus/graphics/Descriptors.hpp"
#include "platypus/graphics/Pipeline.hpp"
#include <vector>
#include <unordered_map>


// TODO: IMPORTANT: Replace below definition with something better!
//  -> That's textures per channel, NOT channel count!
#define PE_MAX_MATERIAL_TEX_CHANNELS 5


namespace platypus
{

    struct MaterialMetadata
    {
        ID_t assetID = NULL_ID;
        float specularStrength = 0.0f;
        float shininess = 0.0f;
        Vector2f textureOffset;
        Vector2f textureScale;
        uint8_t castShadows = 1;
        uint8_t receiveShadows = 1;
        uint8_t transparent = 0;
        uint8_t shadeless = 0;
        uint8_t persistent = 0;
        ID_t blendmapTextureID = NULL_ID;
        ID_t diffuseTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        ID_t specularTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        ID_t normalTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        char name[asset_metadata_name_size];
    };

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
        bool _transparent = false;
        bool _shadeless = false;
        // TODO: oh my god please PLEASE MAKE THIS SHIT LESS DUMB!
        uint32_t _shadowmapDescriptorIndex = 0;
        uint32_t _sceneDepthDescriptorIndex = 0;

        std::unordered_map<MeshType, MaterialPipelineData*> _pipelines;

        // TODO: Material instance
        MaterialUniformBufferData _uniformBufferData;

        DescriptorSetLayout _descriptorSetLayout;
        std::vector<Buffer*> _uniformBuffers;
        std::vector<DescriptorSet> _descriptorSets;

        std::string _customVertexShaderFilename;
        std::string _customFragmentShaderFilename;

    public:
        // NOTE: All transparent materials use opaque pass's depth buffer as texture!
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
            bool transparent = false,
            bool shadeless = false,
            const std::string& name = "",
            ID_t id = NULL_ID,
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
        void updateSceneDepthDescriptorSet(Texture* pSceneDepthTexture);

        Texture* getBlendmapTexture() const;
        Texture* getDiffuseTexture(size_t channel) const;
        Texture* getSpecularTexture(size_t channel) const;
        Texture* getNormalTexture(size_t channel) const;
        std::vector<Texture*> getTextures() const;

        ID_t getDiffuseTextureID(size_t channel) const;
        ID_t getSpecularTextureID(size_t channel) const;
        ID_t getNormalTextureID(size_t channel) const;

        void setLightingProperties(float specularStrength, float shininess, bool shadeless);
        void setTextureProperties(const Vector2f& textureOffset, const Vector2f& textureScale);

        Pipeline* getPipeline(MeshType meshType);

        virtual void writeToMetadataBuffer(
            std::vector<char>& targetBuffer
        ) const override;

        static Material* create_from_metadata_buffer(
            AssetManager* pAssetManager,
            const std::vector<char>& targetBuffer,
            size_t bufferPos
        );

        static size_t get_serialized_metadata_size();

        inline ID_t getBlendmapTextureID() const { return _blendmapTextureID; }
        inline const ID_t* getDiffuseTextureIDs() const { return _diffuseTextureIDs; }
        inline const ID_t* getSpecularTextureIDs() const { return _specularTextureIDs; }
        inline const ID_t* getNormalTextureIDs() const { return _normalTextureIDs; }

        inline bool hasBlendmap() const { return _blendmapTextureID != NULL_ID; }
        inline bool hasNormalMap() const { return _normalTextureIDs[0] != NULL_ID; }

        inline size_t getTotalTextureCount() const
        {
            return _diffuseTextureCount +
                _specularTextureCount +
                _normalTextureCount +
                (_blendmapTextureID != NULL_ID ? 1 : 0) +
                (_receiveShadows ? 1 : 0) +
                (_transparent ? 1 : 0);
        }
        inline float getSpecularStrength() const { return _uniformBufferData.lightingProperties.x; }
        inline float getShininess() const { return _uniformBufferData.lightingProperties.y; }
        inline bool castsShadows() const { return _castShadows; }
        inline bool receivesShadows() const { return _receiveShadows; }
        inline bool isTransparent() const { return _transparent; }
        inline bool isShadeless() const { return _uniformBufferData.lightingProperties.z; }
        inline Vector2f getTextureOffset() const { return { _uniformBufferData.textureProperties.x, _uniformBufferData.textureProperties.y }; }
        inline Vector2f getTextureScale() const { return { _uniformBufferData.textureProperties.z, _uniformBufferData.textureProperties.w };; }

        inline const DescriptorSetLayout& getDescriptorSetLayout() const { return _descriptorSetLayout; }
        inline const std::vector<DescriptorSet> getDescriptorSets() const { return _descriptorSets; }

        void warnUnassigned(const std::string& beginStr);

    private:
        void updateDescriptorSetTexture(Texture* pTexture, uint32_t descriptorIndex);
        void validateTextureCounts();
        void createDescriptorSetLayout();
        // NOTE: This updates all uniform buffers for all possible frames in flight,
        // not sure should we be doing that here...
        void updateUniformBuffers(size_t frame);

        // Returns compiled shader filename depending on given properties
        std::string getShaderFilename(uint32_t shaderStage, MeshType meshType);
    };
}
