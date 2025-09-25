#include "AssetManager.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/assets/SkeletalAnimationData.h"
#include "platypus/core/Debug.h"
#include "platypus/utils/modelLoading/ModelLoading.h"
#include "platypus/Common.h"


namespace platypus
{
    AssetManager::AssetManager()
    {
        // Create black and white default textures
        PE_ubyte whitePixels[4] = { 255, 255, 255, 255 };
        Image* pWhiteImage = createImage(whitePixels, 1, 1, 4);
        _persistentAssets[pWhiteImage->getID()] = pWhiteImage;

        PE_ubyte blackPixels[4] = { 0, 0, 0, 255 };
        Image* pBlackImage = createImage(blackPixels, 1, 1, 4);
        _persistentAssets[pBlackImage->getID()] = pBlackImage;

        PE_ubyte zeroPixels[4] = { 0, 0, 0, 0 };
        Image* pZeroImage = createImage(zeroPixels, 1, 1, 4);
        _persistentAssets[pZeroImage->getID()] = pZeroImage;

        TextureSampler defaultTextureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            1,
            0
        );
        _pWhiteTexture = createTexture(
            pWhiteImage->getID(),
            ImageFormat::R8G8B8A8_SRGB,
            defaultTextureSampler
        );
        _pBlackTexture = createTexture(
            pBlackImage->getID(),
            ImageFormat::R8G8B8A8_SRGB,
            defaultTextureSampler
        );
        _pZeroTexture = createTexture(
            pZeroImage->getID(),
            ImageFormat::R8G8B8A8_SRGB,
            defaultTextureSampler
        );
        _persistentAssets[_pWhiteTexture->getID()] = _pWhiteTexture;
        _persistentAssets[_pBlackTexture->getID()] = _pBlackTexture;
        _persistentAssets[_pZeroTexture->getID()] = _pZeroTexture;
    }

    AssetManager::~AssetManager()
    {
        std::unordered_map<ID_t, Asset*>::iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
            delete it->second;

        _assets.clear();
        _persistentAssets.clear();
    }

    void AssetManager::destroyAssets()
    {
        std::unordered_map<ID_t, Asset*>::iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
        {
            if (_persistentAssets.find(it->first) == _persistentAssets.end())
                delete it->second;
        }
        _assets.clear();

        // Re add persistent assets to assets
        std::unordered_map<ID_t, Asset*>::iterator persistentIt;
        for (persistentIt = _persistentAssets.begin(); persistentIt != _persistentAssets.end(); ++persistentIt)
            _assets[persistentIt->first] = persistentIt->second;

        Debug::log("Assets destroyed");
    }

    Image* AssetManager::createImage(PE_ubyte* pData, int width, int height, int channels)
    {
        bool failure = false;
        if (!pData)
        {
            Debug::log(
                "@AssetManager::createImage "
                "Provided data was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            failure = true;
        }
        if (width <= 0 || height <= 0)
        {
            Debug::log(
                "@AssetManager::createImage "
                "Invalid image dimensions: " + std::to_string(width) + "x"+ std::to_string(height),
                Debug::MessageType::PLATYPUS_ERROR
            );
            failure = true;
        }
        if (width <= 0 || height <= 0)
        {
            Debug::log(
                "@AssetManager::createImage "
                "Invalid channel count: " + std::to_string(channels),
                Debug::MessageType::PLATYPUS_ERROR
            );
            failure = true;
        }
        if (failure)
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        Image* pImage = new Image(pData, width, height, channels);
        _assets[pImage->getID()] = pImage;
        return pImage;
    }

    Image* AssetManager::loadImage(const std::string& filepath)
    {
        Image* pImage = Image::load_image(filepath);
        if (!pImage)
        {
            Debug::log(
                "@AssetManager::loadImage "
                "Failed to load image from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        _assets[pImage->getID()] = pImage;
        return pImage;
    }

    Texture* AssetManager::createTexture(
        ID_t imageID,
        ImageFormat targetFormat,
        const TextureSampler& sampler,
        uint32_t textureAtlasRows
    )
    {
        Image* pImage = (Image*)getAsset(imageID, AssetType::ASSET_TYPE_IMAGE);
        if (!pImage)
        {
            Debug::log(
                "AssetManager::createTexture "
                "Failed to find image with ID: " + std::to_string(imageID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        Texture* pTexture = new Texture(
            pImage,
            targetFormat,
            sampler,
            textureAtlasRows
        );
        _assets[pTexture->getID()] = pTexture;
        return pTexture;
    }

    Texture* AssetManager::loadTexture(
        const std::string& filepath,
        ImageFormat targetFormat,
        const TextureSampler& sampler,
        uint32_t textureAtlasRows
    )
    {
        Image* pImage = loadImage(filepath);
        if (!pImage)
        {
            Debug::log(
                "@AssetManager::loadTexture "
                "Failed to load texture",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        return createTexture(pImage->getID(), targetFormat, sampler, textureAtlasRows);
    }

    Material* AssetManager::createMaterial(
        ID_t diffuseTextureID,
        ID_t specularTextureID,
        ID_t normalTextureID,
        float specularStrength,
        float shininess,
        bool shadeless
    )
    {

        std::unordered_map<ID_t, Asset*>::const_iterator diffuseTextureIt = _assets.find(diffuseTextureID);
        std::unordered_map<ID_t, Asset*>::const_iterator specularTextureIt = _assets.find(specularTextureID);

        Texture* pDiffuseTexture = nullptr;
        Texture* pSpecularTexture = nullptr;
        if (diffuseTextureIt != _assets.end())
        {
            AssetType foundAssetType = _assets[diffuseTextureID]->getType();
            if (foundAssetType != AssetType::ASSET_TYPE_TEXTURE)
            {
                Debug::log(
                    "@AssetManager::createMaterial "
                    "Invalid asset type: " + asset_type_to_string(foundAssetType) + " " +
                    "for diffuse texture",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            pDiffuseTexture = (Texture*)_assets[diffuseTextureID];
        }
        else
        {
            pDiffuseTexture = _pWhiteTexture;
        }

        if (specularTextureIt != _assets.end())
        {
            AssetType foundAssetType = _assets[specularTextureID]->getType();
            if (foundAssetType != AssetType::ASSET_TYPE_TEXTURE)
            {
                Debug::log(
                    "@AssetManager::createMaterial "
                    "Invalid asset type: " + asset_type_to_string(foundAssetType) + " " +
                    "for specular texture",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            pSpecularTexture = (Texture*)_assets[specularTextureID];
        }
        else
        {
            pSpecularTexture = _pBlackTexture;
        }

        Material* pMaterial = new Material(
            pDiffuseTexture->getID(),
            pSpecularTexture->getID(),
            normalTextureID,
            specularStrength,
            shininess,
            shadeless
        );
        _assets[pMaterial->getID()] = pMaterial;
        return pMaterial;
    }

    TerrainMaterial* AssetManager::createTerrainMaterial(
        ID_t blendmapTextureID,
        std::vector<ID_t> diffuseChannelTextures,
        std::vector<ID_t> specularChannelTextures
    )
    {
        if (diffuseChannelTextures.size() > PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@AssetManager::createTerrainMaterial "
                "Too many diffuse channel textures provided(" + std::to_string(diffuseChannelTextures.size()) + ") "
                "Maximum texture channels: " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (specularChannelTextures.size() > PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@AssetManager::createTerrainMaterial "
                "Too many specular channel textures provided(" + std::to_string(specularChannelTextures.size()) + ") "
                "Maximum texture channels: " + std::to_string(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        ID_t useBlendmapTextureID = blendmapTextureID;
        if (!validateAsset(PLATYPUS_CURRENT_FUNC_NAME, blendmapTextureID, AssetType::ASSET_TYPE_TEXTURE))
            useBlendmapTextureID = _pBlackTexture->getID();

        ID_t defaultTextureID = _pBlackTexture->getID();
        // Validate diffuse and specular textures. Use default(black texture) if invalid or not given.
        std::vector<ID_t> useDiffuseChannelTextureIDs(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS);
        std::vector<ID_t> useSpecularChannelTextureIDs(PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS);
        for (size_t textureChannel = 0; textureChannel < PE_MAX_TERRAIN_MATERIAL_TEX_CHANNELS; ++textureChannel)
        {
            if (textureChannel >= diffuseChannelTextures.size())
            {
                useDiffuseChannelTextureIDs[textureChannel] = defaultTextureID;
            }
            else
            {
                ID_t diffuseTextureID = diffuseChannelTextures[textureChannel];
                if (!validateAsset(PLATYPUS_CURRENT_FUNC_NAME, diffuseTextureID, AssetType::ASSET_TYPE_TEXTURE))
                    diffuseTextureID = defaultTextureID;

                useDiffuseChannelTextureIDs[textureChannel] = diffuseTextureID;
            }

            if (textureChannel >= specularChannelTextures.size())
            {
                useSpecularChannelTextureIDs[textureChannel] = defaultTextureID;
            }
            else
            {
                ID_t specularTextureID = specularChannelTextures[textureChannel];
                if (!validateAsset(PLATYPUS_CURRENT_FUNC_NAME, specularTextureID, AssetType::ASSET_TYPE_TEXTURE))
                    specularTextureID = defaultTextureID;

                useSpecularChannelTextureIDs[textureChannel] = specularTextureID;
            }
        }

        TerrainMaterial* pTerrainMaterial = new TerrainMaterial(
            useBlendmapTextureID,
            useDiffuseChannelTextureIDs.data(),
            useDiffuseChannelTextureIDs.size(),
            useSpecularChannelTextureIDs.data(),
            useSpecularChannelTextureIDs.size()
        );
        _assets[pTerrainMaterial->getID()] = pTerrainMaterial;
        return pTerrainMaterial;
    }

    Mesh* AssetManager::createMesh(
        const VertexBufferLayout& vertexBufferLayout,
        const std::vector<float>& vertexData,
        const std::vector<uint32_t>& indexData
    )
    {
        Buffer* pVertexBuffer = new Buffer(
            (void*)vertexData.data(),
            sizeof(float),
            vertexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        Buffer* pIndexBuffer = new Buffer(
            (void*)indexData.data(),
            sizeof(uint32_t),
            indexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        Mesh* pMesh = new Mesh(
            vertexBufferLayout,
            pVertexBuffer,
            pIndexBuffer,
            Matrix4f(1.0f)
        );
        _assets[pMesh->getID()] = pMesh;
        return pMesh;
    }

    Model* AssetManager::loadModel(const std::string& filepath)
    {
        std::vector<MeshData> loadedMeshes;
        std::vector<KeyframeAnimationData> loadedAnimations;
        if (!load_gltf_model(filepath, loadedMeshes, loadedAnimations))
        {
            Debug::log(
                "@AssetManager::loadModel "
                "Failed to load model using filepath: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        std::vector<Mesh*> createdMeshes;
        for (const MeshData& meshData : loadedMeshes)
        {
            Buffer* pVertexBuffer = new Buffer(
                (void*)meshData.vertexBufferData.rawData.data(),
                meshData.vertexBufferData.elementSize,
                meshData.vertexBufferData.length,
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );
            MeshBufferData useIndexBuffer = meshData.indexBufferData[0];
            Buffer* pIndexBuffer = new Buffer(
                (void*)useIndexBuffer.rawData.data(),
                useIndexBuffer.elementSize,
                useIndexBuffer.length,
                BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );

            Mesh* pMesh = new Mesh(
                meshData.vertexBufferLayout,
                pVertexBuffer,
                pIndexBuffer,
                meshData.transformationMatrix
            );
            _assets[pMesh->getID()] = pMesh;
            createdMeshes.push_back(pMesh);
        }
        Model* pModel = new Model(createdMeshes);
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    Model* AssetManager::loadModel(
        const std::string& filepath,
        std::vector<KeyframeAnimationData>& outAnimations
    )
    {
        std::vector<MeshData> loadedMeshes;
        if (!load_gltf_model(filepath, loadedMeshes, outAnimations))
        {
            Debug::log(
                "@AssetManager::loadModel "
                "Failed to load model using filepath: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        std::vector<Mesh*> createdMeshes;
        for (const MeshData& meshData : loadedMeshes)
        {
            Buffer* pVertexBuffer = new Buffer(
                (void*)meshData.vertexBufferData.rawData.data(),
                meshData.vertexBufferData.elementSize,
                meshData.vertexBufferData.length,
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );
            MeshBufferData useIndexBuffer = meshData.indexBufferData[0];
            Buffer* pIndexBuffer = new Buffer(
                (void*)useIndexBuffer.rawData.data(),
                useIndexBuffer.elementSize,
                useIndexBuffer.length,
                BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );

            Mesh* pMesh = new Mesh(
                meshData.vertexBufferLayout,
                pVertexBuffer,
                pIndexBuffer,
                meshData.transformationMatrix,
                meshData.bindPose
            );
            _assets[pMesh->getID()] = pMesh;
            createdMeshes.push_back(pMesh);
        }
        Model* pModel = new Model(createdMeshes);
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    TerrainMesh* AssetManager::createTerrainMesh(
        float tileSize,
        const std::vector<float>& heightmapData,
        bool dynamic
    )
    {
        TerrainMesh* pTerrainMesh = new TerrainMesh(tileSize, heightmapData, dynamic);
        _assets[pTerrainMesh->getID()] = pTerrainMesh;
        return pTerrainMesh;
    }

    SkeletalAnimationData* AssetManager::createSkeletalAnimation(
        const KeyframeAnimationData& keyframes
    )
    {
        SkeletalAnimationData* pAnimationData = new SkeletalAnimationData(
            keyframes
        );
        _assets[pAnimationData->getID()] = pAnimationData;
        return pAnimationData;
    }

    Font* AssetManager::loadFont(const std::string& filepath, unsigned int pixelSize)
    {
        Font* pFont = new Font;
        if (!pFont->load(filepath, pixelSize))
        {
            Debug::log(
                "@AssetManager::loadFont "
                "Failed to load font from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            delete pFont;
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        _assets[pFont->getID()] = pFont;
        return pFont;
    }

    Asset* AssetManager::getAsset(ID_t assetID, AssetType type) const
    {
        std::unordered_map<ID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it == _assets.end())
        {
            Debug::log(
                "@AssetManager::getAsset "
                "Failed to find asset of type: " + asset_type_to_string(type) + " "
                "with id: " + std::to_string(assetID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        if (it->second->getType() != type)
        {
            Debug::log(
                "@AssetManager::getAsset "
                "Requested asset of type " + asset_type_to_string(type) + " "
                "Found asset with id: " + std::to_string(assetID) + " " +
                "but the asset's type was " + asset_type_to_string(it->second->getType()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        return it->second;
    }

    std::vector<Asset*> AssetManager::getAssets(AssetType type) const
    {
        std::vector<Asset*> foundAssets;
        for (const auto& asset : _assets)
        {
            if (asset.second->getType() == type)
                foundAssets.push_back(asset.second);
        }
        return foundAssets;
    }

    bool AssetManager::validateAsset(
        const char* callLocation,
        ID_t assetID,
        AssetType requiredType
    )
    {
        const std::string callLocationStr(callLocation);
        const std::string locationStr(PLATYPUS_CURRENT_FUNC_NAME);
        std::unordered_map<ID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it == _assets.end())
        {
            Debug::log(
                "@AssetManager::" + locationStr + "(" + locationStr + ") "
                "Failed to find asset with ID: " + std::to_string(assetID) + " "
                "with type: " + asset_type_to_string(requiredType),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        if (_assets[assetID]->getType() != requiredType)
        {
            Debug::log(
                "@AssetManager::" + locationStr + "(" + locationStr + ") "
                "Invalid asset type: " + asset_type_to_string(_assets[assetID]->getType()) + " "
                "for asset ID: " + std::to_string(assetID) + " "
                "required type was: " + asset_type_to_string(requiredType),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }
}
