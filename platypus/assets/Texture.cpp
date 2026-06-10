#include "Texture.hpp"
#include "platypus/core/Application.hpp"
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


    void Texture::recreate(const Image* pImage, const TextureSampler* pSampler)
    {
        destroy();
        _pSampler = pSampler;
        create(pImage);

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        std::vector<Asset*> materialAssets = pAssetManager->getAssets(
            AssetType::ASSET_TYPE_MATERIAL
            //bool excludeInternalDefaults = false,
            //bool excludeNonSerializable = true
        );
        for (Asset* pAsset : materialAssets)
        {
            Material* pMaterial = reinterpret_cast<Material*>(pAsset);
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
                    pMaterial->destroyPipeline();
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


    /*
        Serialized format (in order):
            ID_t assetID = NULL_ID;
            ID_t imageID = NULL_ID;
            TextureSamplerFilterMode filterMode;
            TextureSamplerAddressMode addressMode;
            uint8_t useMipmapping = 0;
            uint8_t persistent = 0;
            char name[metadata_name_size];
    */
    void Texture::writeToMetadataBuffer(
        std::vector<char>& targetBuffer
    ) const
    {
        PLATYPUS_ASSERT(_name.size() <= asset_metadata_name_size);
        PLATYPUS_ASSERT(_pImage);
        PLATYPUS_ASSERT(_pImage->getID() != NULL_UUID);

        const size_t prevSize = targetBuffer.size();
        targetBuffer.resize(prevSize + get_serialized_metadata_size());
        char* pBuf = targetBuffer.data() + prevSize;

        memcpy(pBuf, &_id, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        const UUID_t imageID = _pImage->getID();
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

        PLATYPUS_ASSERT(get_serialized_metadata_size());
    }

    Texture* Texture::create_from_metadata_buffer(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    )
    {
        PLATYPUS_ASSERT((bufferPos  + get_serialized_metadata_size()) <= targetBuffer.size());

        UUID_t id;
        UUID_t imageID;
        TextureSamplerFilterMode samplerFilterMode;
        TextureSamplerAddressMode samplerAddressMode;
        uint8_t useMipmapping;
        uint8_t persistent;
        char name[asset_metadata_name_size];

        const char* pBuf = targetBuffer.data() + bufferPos;

        memcpy(&id, pBuf, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        memcpy(&imageID, pBuf + pos, sizeof(UUID_t));
        pos += sizeof(UUID_t);

        memcpy(&samplerFilterMode, pBuf + pos, sizeof(TextureSamplerFilterMode));
        pos += sizeof(TextureSamplerFilterMode);

        memcpy(&samplerAddressMode, pBuf + pos, sizeof(TextureSamplerAddressMode));
        pos += sizeof(TextureSamplerAddressMode);

        memcpy(&useMipmapping, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&persistent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&name, pBuf + pos, asset_metadata_name_size);
        pos += asset_metadata_name_size;

        PLATYPUS_ASSERT(get_serialized_metadata_size());

        Texture* pTexture = pAssetManager->createTexture(
            imageID,
            samplerFilterMode,
            samplerAddressMode,
            static_cast<bool>(useMipmapping),
            std::string(name),
            id
        );

        if (persistent)
            pAssetManager->makePersistent(pTexture);

        return pTexture;
    }

    size_t Texture::get_serialized_metadata_size()
    {
        return sizeof(UUID_t) * 2 +
            sizeof(TextureSamplerFilterMode) +
            sizeof(TextureSamplerAddressMode) +
            sizeof(uint8_t) * 2 +
            asset_metadata_name_size;
    }
}
