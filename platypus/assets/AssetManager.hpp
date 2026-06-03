#pragma once

#include "Asset.hpp"
#include "Image.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "Material.hpp"
#include "Font.hpp"
#include "SkeletalAnimationData.hpp"
#include <unordered_map>
#include <vector>


namespace platypus
{
    class AssetManager
    {
    private:
        size_t _uuidPool = 0;
        std::unordered_map<UUID_t, Asset*> _assets;
        // NOTE: Why the fuck isn't _persistentAssets a set, like _defaultAssets or something?
        std::unordered_map<UUID_t, Asset*> _persistentAssets;
        std::set<UUID_t> _defaultAssets;

        // NOTE: Currently these exist throughout the lifetime of the program
        //  -> there's only a couple of sampler property combinations so.. why the fuck not...
        std::vector<TextureSampler*> _textureSamplers;

        // TODO: Maybe get rid of the image data after the textures are created...
        //  -> images are never used after that?
        Image* _pWhiteImage = nullptr;
        Image* _pBlackImage = nullptr;
        Image* _pZeroImage = nullptr;
        Image* _pErrorImage = nullptr;

        Texture* _pWhiteTexture = nullptr;
        Texture* _pBlackTexture = nullptr;
        Texture* _pZeroTexture = nullptr;
        Texture* _pErrorTexture = nullptr;

        Material* _pErrorMaterial = nullptr;

        Model* _pErrorModel = nullptr;
        Mesh* _pErrorMesh = nullptr;

        std::vector<std::string> _errors;

    public:
        AssetManager();
        ~AssetManager();
        void createDefaultAssets();
        void destroyAssets();
        void destroyAsset(UUID_t assetID);
        void destroyAsset(const std::string& assetName);

        Image* createImage(PE_ubyte* pData, int width, int height, int channels, ImageFormat format);
        Image* loadImage(
            const std::string& filepath,
            ImageFormat format,
            const std::string& name = "",
            UUID_t id = NULL_UUID
        );
        Texture* createTexture(
            UUID_t imageID,
            const TextureSampler* pSampler,
            const std::string& name = "",
            UUID_t id = NULL_UUID
        );
        Texture* createTexture(
            UUID_t imageID,
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            bool useMipmapping,
            const std::string& name = "",
            UUID_t id = NULL_UUID
        );
        Texture* loadTexture(
            const std::string& filepath,
            ImageFormat format,
            const TextureSampler* pSampler,
            const std::string& name = "",
            UUID_t id = NULL_UUID
        );
        Material* createMaterial(
            UUID_t blendmapTextureID,
            std::vector<UUID_t> diffuseTextureIDs,
            std::vector<UUID_t> specularTextureIDs,
            std::vector<UUID_t> normalTextureIDs,
            float specularStrength,
            float shininess,
            const Vector2f& textureOffset,
            const Vector2f& textureScale,
            bool castShadows,
            bool receiveShadows,
            bool transparent,
            bool shadeless,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            const std::string& customVertexShaderFilename = "",
            const std::string& customFragmentShaderFilename = ""
        );
        Material* createMaterial(
            UUID_t blendmapTextureID,
            std::vector<UUID_t> diffuseTextureIDs,
            std::vector<UUID_t> specularTextureIDs,
            std::vector<UUID_t> normalTextureIDs,
            float specularStrength = 0.625f,
            float shininess = 16.0f
        );
        Mesh* createMesh(
            const VertexBufferLayout& vertexBufferLayout,
            const std::vector<float>& vertexData,
            const std::vector<uint32_t>& indexData,
            MeshType meshType
        );
        // TODO: Way to load "scenes" containing skinned and non skinned meshes
        Model* loadStaticModel(const std::string& filepath, bool instanced = true);
        Model* loadSkinnedModel(
            const std::string& filepath,
            std::vector<KeyframeAnimationData>& outAnimations
        );
        // Can be used to load both, static and skinned meshes
        // TODO: instanced property should be per mesh and NOT per model!
        //  -> Fix everywhere!
        Model* loadModel(
            const std::string& filepath,
            bool instanced,
            const std::string& name,
            UUID_t modelID = NULL_UUID,
            std::vector<UUID_t> meshIDs = { }
        );
        Mesh* createTerrainMesh(
            float tileSize,
            const std::vector<float>& heightmapData,
            bool dynamic,
            bool generateTangents
        );

        SkeletalAnimationData* createSkeletalAnimation(
            const KeyframeAnimationData& keyframes
        );
        Font* loadFont(const std::string& filepath, unsigned int pixelSize);

        const TextureSampler* createTextureSampler(
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            bool useMipmapping
        );

        const TextureSampler* getTextureSampler(
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            bool useMipmapping
        ) const;

        const TextureSampler* getOrCreateTextureSampler(
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            bool useMipmapping
        );

        bool assetExists(UUID_t assetID) const;
        bool assetExists(UUID_t assetID, AssetType type) const;
        bool assetExists(const std::string& assetName) const;
        Asset* getAsset(UUID_t assetID) const;
        Asset* getAsset(UUID_t assetID, AssetType type) const;
        // NOTE: This is pretty fucking inefficient!
        Asset* getAsset(const std::string& assetName) const;
        std::vector<Asset*> getAssets(
            bool excludeInternalDefaults = false,
            bool excludeNonSerializable = true
        ) const;
        std::vector<Asset*> getAssets(
            AssetType type,
            bool excludeInternalDefaults = false,
            bool excludeNonSerializable = true
        ) const;

        void makePersistent(Asset* pAsset);
        // For adding asset that wasn't created using the AssetManager.
        // This piece of shit requires all textures, including framebuffer attachments,
        // used for all render passes to be "reqistered assets" atm so need to have some
        // way of differentiating user created assets and "engine's defaults".
        // NOTE: All external defaults are also persistent!
        void addExternalDefaultAsset(Asset* pAsset);

        bool isPersistent(UUID_t assetID) const;

        std::vector<char> serialize(
            const std::vector<Asset*>& assets
        );

        // Returns the position after the header in serializedData (ie where the actual data begins)
        size_t deserializeHeader(
            const std::vector<char>& serializedData,
            size_t* pImageCount,
            size_t* pTextureCount,
            size_t* pMaterialCount,
            size_t* pModelCount
        ) const;

        // This is to be able to load one asset per fram (in order to get some kind of loading
        // screen working since don't know what to do with web parallelization yet...)
        //
        // *The last read pos is stored in the bufferReadEndPos to be able to start reading the
        // next asset in the serializedData
        Asset* deserialize(
            size_t imageCount,
            size_t textureCount,
            size_t materialCount,
            size_t modelCount,
            const std::vector<char>& serializedData,
            size_t bufferReadPos,
            size_t& bufferReadEndPos
        );

        std::unordered_map<std::string, Asset*> deserialize(
            const std::vector<char>& serializedData,
            size_t& lastReadPos
        );

        std::vector<std::string> popErrors();

        // TODO: Figure out better way to deal with asset names!
        // Checking is name available is horribly slow atm!
        bool nameAvailable(const std::string& name) const;

        inline size_t getAssetTotalCount() const { return _assets.size(); }
        inline Texture* getWhiteTexture() const { return _pWhiteTexture; }
        inline Texture* getBlackTexture() const { return _pBlackTexture; }
        inline Texture* getZeroTexture() const { return _pZeroTexture; }
        inline Texture* getErrorTexture() const { return _pErrorTexture; }

        inline Material* getErrorMaterial() const { return _pErrorMaterial; }
        inline Mesh* getErrorMesh() const { return _pErrorMesh; }

        inline size_t getUUIDPool() const { return _uuidPool; }

    private:
        bool validateAsset(
            const char* callLocation,
            UUID_t assetID,
            AssetType requiredType
        );

        void destroyDefaultAssets();
    };
}
