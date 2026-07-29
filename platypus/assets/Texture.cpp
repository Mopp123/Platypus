#include "Texture.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/assets/AssetManager.hpp"
#include "platypus/core/Debug.hpp"
#include "AssetManager.hpp"

namespace platypus
{
    std::string texture_sampler_filter_mode_to_string(TextureSamplerFilterMode mode)
    {
        switch (mode)
        {
            case TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR : return "SAMPLER_FILTER_MODE_LINEAR";
            case TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR: return "SAMPLER_FILTER_MODE_NEAR";
        }
    }


    std::string texture_sampler_address_mode_to_string(TextureSamplerAddressMode mode)
    {
        switch (mode)
        {
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT: return "SAMPLER_ADDRESS_MODE_REPEAT";
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: return "SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE";
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return "SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER";
        }
    }


    Texture::Texture(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    ) :
        Asset(pAssetManager->getUUIDPool())
    {
        const size_t serializedSize = getSerializedSize();
        PLATYPUS_ASSERT((bufferPos + serializedSize) <= targetBuffer.size());

        const char* pBuf = targetBuffer.data() + bufferPos;
        memcpy(&_id, pBuf, sizeof(UUID_t));
        UUID::occupy(_id, _uuidPool);
        size_t pos = sizeof(UUID_t);

        memcpy(&_type, pBuf + pos, sizeof(AssetType));
        pos += sizeof(AssetType);

        memcpy(&_customFlags, pBuf + pos, asset_metadata_custom_flags_size);
        pos += asset_metadata_custom_flags_size;

        UUID_t imageUUID;
        memcpy(&imageUUID, pBuf + pos, sizeof(UUID_t));
        pos += sizeof(UUID_t);
        pAssetManager->addToDeserializationTextureImageUUIDQuery(_id, imageUUID);

        TextureSamplerFilterMode filterMode;
        memcpy(&filterMode, pBuf + pos, sizeof(TextureSamplerFilterMode));
        pos += sizeof(TextureSamplerFilterMode);

        TextureSamplerAddressMode addressMode;
        memcpy(&addressMode, pBuf + pos, sizeof(TextureSamplerAddressMode));
        pos += sizeof(TextureSamplerAddressMode);

        uint8_t useMipmapping;
        memcpy(&useMipmapping, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        _pSampler = pAssetManager->getOrCreateTextureSampler(
            filterMode,
            addressMode,
            static_cast<bool>(useMipmapping)
        );

        // TODO: Put all "pure Asset" related stuff in the beginning instead!
        uint8_t persistent;
        memcpy(&persistent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);
        _persistent = static_cast<bool>(persistent);

        char name[asset_metadata_name_size];
        memcpy(name, pBuf + pos, asset_metadata_name_size);
        _name = std::string(name);

        pAssetManager->addExternalAsset(this);
        if (_persistent)
            pAssetManager->makePersistent(this);
    }

    void Texture::recreate(const Image* pImage, const TextureSampler* pSampler)
    {
        destroy();
        _pSampler = pSampler;
        create(pImage);

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();

        // Update all materials using this texture so the texture change can take effect!
        std::vector<Asset*> materialAssets = pAssetManager->getAssets(
            AssetType::ASSET_TYPE_MATERIAL
            //bool excludeInternalDefaults = false,
            //bool excludeNonSerializable = true
        );
        for (Asset* pAsset : materialAssets)
        {
            PLATYPUS_ASSERT(pAsset);
            Material* pMaterial = dynamic_cast<Material*>(pAsset);
            PLATYPUS_ASSERT(pMaterial);
            const std::vector<Texture*> materialTextures = pMaterial->getTextures();
            for (Texture* pTexture : materialTextures)
            {
                if (pTexture == this)
                {
                    // NOTE: Currently freeing all batches here!
                    // TODO: Find and free only batches using this Material!!
                    //  -> ITS' SLOW AS FUCK TO RECREATE ALL BATCHES!
                    Debug::log(
                        "Doing very dumb shit here atm! TODO: Make this better!",
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_WARNING
                    );
                    pMaterial->destroyPipelines();
                    pMaterial->recreateExistingPipelines();
                    pMaterial->destroyShaderResources();
                    pMaterial->createShaderResources();

                    MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
                    pMasterRenderer->getBatcher().freeBatches();
                    break;
                }
            }
        }
    }

    void Texture::recreate(const Image* pImage)
    {
        recreate(pImage, _pSampler);
    }

    /*
        Serialized format:
            ID_t assetID
            AssetType type
            uint64_t customFlags
            ID_t imageID
            TextureSamplerFilterMode filterMode
            TextureSamplerAddressMode addressMode
            uint8_t useMipmapping
            uint8_t persistent
            char name[metadata_name_size]
    */
    void Texture::serialize(
        std::vector<char>& targetBuffer
    ) const
    {
        PLATYPUS_ASSERT(_name.size() <= asset_metadata_name_size);
        PLATYPUS_ASSERT(_pImage);
        PLATYPUS_ASSERT(_pImage->getID() != NULL_UUID);

        const size_t serializedSize = getSerializedSize();
        const size_t prevSize = targetBuffer.size();
        targetBuffer.resize(prevSize + serializedSize);
        char* pBuf = targetBuffer.data() + prevSize;

        memcpy(pBuf, &_id, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        memcpy(pBuf + pos, &_type, sizeof(AssetType));
        pos += sizeof(AssetType);

        memcpy(pBuf + pos, &_customFlags, asset_metadata_custom_flags_size);
        pos += asset_metadata_custom_flags_size;

        UUID_t imageID = _pImage->getID();
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        if (_pImage == pAssetManager->getErrorImage())
            imageID = NULL_UUID;

        memcpy(pBuf + pos, &imageID, sizeof(UUID_t));
        pos += sizeof(UUID_t);

        const TextureSamplerFilterMode samplerFilterMode = _pSampler->getFilterMode();
        memcpy(pBuf + pos, &samplerFilterMode, sizeof(TextureSamplerFilterMode));
        pos += sizeof(TextureSamplerFilterMode);

        const TextureSamplerAddressMode samplerAddressMode = _pSampler->getAddressMode();
        memcpy(pBuf + pos, &samplerAddressMode, sizeof(TextureSamplerAddressMode));
        pos += sizeof(TextureSamplerAddressMode);

        uint8_t useMipmapping = static_cast<uint8_t>(_pSampler->isMipmapped());
        memcpy(pBuf + pos, &useMipmapping, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint8_t persistent = static_cast<const uint8_t>(_persistent);
        memcpy(pBuf + pos, &persistent, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        // Clear the buf for the longest possible name
        memset(pBuf + pos, 0, asset_metadata_name_size);
        // Write the name
        memcpy(pBuf + pos, _name.data(), _name.size());
        pos += asset_metadata_name_size;

        PLATYPUS_ASSERT(pos == serializedSize);
    }

    size_t Texture::getSerializedSize() const
    {
        return sizeof(UUID_t) +
            sizeof(AssetType) +
            asset_metadata_custom_flags_size +
            sizeof(UUID_t) +
            sizeof(TextureSamplerFilterMode) +
            sizeof(TextureSamplerAddressMode) +
            sizeof(uint8_t) * 2 +
            asset_metadata_name_size;
    }

    void Texture::fixMaterialsOnDestruction()
    {
        // NOTE: Was unable to get this working properly when destroying "default assets".
        // That doesn't matter atm since default assets should persist through lifetime of app.
        // BUT: Wasn't really sure why that didn't work -> MIGHT CAUSE ISSUES LATER IF THIS IS
        // WORKING JUST BY ACCIDENT ATM!
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        const Asset* pErrorTextureAsset = pAssetManager->getErrorTexture();
        if (pErrorTextureAsset)
        {
            const UUID_t errorTextureID = pErrorTextureAsset->getID();

            std::vector<Asset*> materials = pAssetManager->getAssets(
                AssetType::ASSET_TYPE_MATERIAL,
                true, // excludeInternalDefaults,
                false // excludeNonSerializable
            );
            for (Asset* pMaterialAsset : materials)
            {
                PLATYPUS_ASSERT(pMaterialAsset);
                Material* pMaterial = dynamic_cast<Material*>(pMaterialAsset);
                const UUID_t blendmapTextureID = pMaterial->getBlendmapTextureID();
                const UUID_t* pDiffuseTextureIDs = pMaterial->getDiffuseTextureIDs();
                const UUID_t* pSpecularTextureIDs = pMaterial->getSpecularTextureIDs();
                const UUID_t* pNormalTextureIDs = pMaterial->getNormalTextureIDs();

                if (blendmapTextureID == _id)
                    pMaterial->setBlendmapTexture(errorTextureID);

                for (size_t slot = 0; slot < PE_MATERIAL_TEX_CHANNEL_SLOTS; ++slot)
                {
                    UUID_t diffuseTextureID = pDiffuseTextureIDs[slot];
                    UUID_t specularTextureID = pSpecularTextureIDs[slot];
                    UUID_t normalTextureID = pNormalTextureIDs[slot];

                    if (diffuseTextureID == _id)
                        pMaterial->setDiffuseTexture(errorTextureID, slot);
                    if (specularTextureID == _id)
                        pMaterial->setSpecularTexture(errorTextureID, slot);
                    if (normalTextureID == _id)
                        pMaterial->setNormalTexture(errorTextureID, slot);
                }
            }
        }
    }
}
