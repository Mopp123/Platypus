#pragma once

#include "Asset.h"
#include "Image.h"
#include "Mesh.h"
#include "TerrainMesh.hpp"
#include "Model.h"
#include "Texture.h"
#include "Material.h"
#include "Font.h"
#include "SkeletalAnimationData.h"
#include <unordered_map>
#include <vector>


namespace platypus
{
    class AssetManager
    {
    private:
        std::unordered_map<ID_t, Asset*> _assets;
        std::unordered_map<ID_t, Asset*> _persistentAssets;

        Texture* _pWhiteTexture = nullptr;
        Texture* _pBlackTexture = nullptr;
        Texture* _pZeroTexture = nullptr;

    public:
        AssetManager();
        ~AssetManager();
        void destroyAssets();

        Image* createImage(PE_ubyte* pData, int width, int height, int channels, ImageFormat format);
        Image* loadImage(const std::string& filepath, ImageFormat format);
        Texture* createTexture(
            ID_t imageID,
            const TextureSampler& sampler,
            uint32_t textureAtlasRows = 1
        );
        Texture* loadTexture(
            const std::string& filepath,
            ImageFormat format,
            const TextureSampler& sampler,
            uint32_t textureAtlasRows = 1
        );
        Material* createMaterial(
            MaterialType type,
            ID_t blendmapTextureID,
            std::vector<ID_t> diffuseTextureIDs,
            std::vector<ID_t> specularTextureIDs,
            std::vector<ID_t> normalTextureIDs,
            float specularStrength = 0.625f,
            float shininess = 16.0f,
            bool shadeless = false
        );
        Mesh* createMesh(
            const VertexBufferLayout& vertexBufferLayout,
            const std::vector<float>& vertexData,
            const std::vector<uint32_t>& indexData
        );
        Model* loadModel(const std::string& filepath);
        Model* loadModel(
            const std::string& filepath,
            std::vector<KeyframeAnimationData>& outAnimations
        );
        TerrainMesh* createTerrainMesh(
            float tileSize,
            const std::vector<float>& heightmapData,
            bool dynamic,
            bool generateTangents
        );

        SkeletalAnimationData* createSkeletalAnimation(
            const KeyframeAnimationData& keyframes
        );
        Font* loadFont(const std::string& filepath, unsigned int pixelSize);

        Asset* getAsset(ID_t assetID, AssetType type) const;
        std::vector<Asset*> getAssets(AssetType type) const;

        inline Texture* getWhiteTexture() const { return _pWhiteTexture; }
        inline Texture* getBlackTexture() const { return _pBlackTexture; }
        inline Texture* getZeroTexture() const { return _pZeroTexture; }

    private:
        bool validateAsset(
            const char* callLocation,
            ID_t assetID,
            AssetType requiredType
        );
    };
}
