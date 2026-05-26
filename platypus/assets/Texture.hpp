#pragma once

#include "Asset.hpp"
#include "Image.hpp"
#include "platypus/graphics/CommandBuffer.hpp"
#include "platypus/graphics/Pipeline.hpp"
#include <string>


namespace platypus
{
    enum class TextureSamplerFilterMode : uint32_t
    {
        SAMPLER_FILTER_MODE_LINEAR,
        SAMPLER_FILTER_MODE_NEAR
    };

    enum class TextureSamplerAddressMode : uint32_t
    {
        SAMPLER_ADDRESS_MODE_REPEAT,
        SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
    };

    std::string texture_sampler_filter_mode_to_string(TextureSamplerFilterMode mode);
    std::string texture_sampler_address_mode_to_string(TextureSamplerAddressMode mode);

    enum class TextureType
    {
        NONE,
        COLOR_TEXTURE,
        DEPTH_TEXTURE
    };

    // NOTE: Atm TextureSampler is supposed to be passed around as const ref!
    // That's why pImpl is shared_ptr in this case
    struct TextureSamplerImpl;
    class TextureSampler
    {
    private:
        TextureSamplerImpl* _pImpl = nullptr;
        TextureSamplerFilterMode _filterMode;
        TextureSamplerAddressMode _addressMode;
        bool _mipmapping = false;

    public:
        // NOTE: Had to add default constructor for editor -> THIS MIGHT FUCK SHIT UP!
        TextureSampler() {}
        TextureSampler(
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            bool mipmapping,
            uint32_t anisotropicFiltering
        );
        ~TextureSampler();
        TextureSampler(const TextureSampler& other) = delete;

        inline TextureSamplerFilterMode getFilterMode() const { return _filterMode; }
        inline TextureSamplerAddressMode getAddressMode() const { return _addressMode; }
        inline const TextureSamplerImpl* getImpl() const { return _pImpl; }
        inline bool isMipmapped() const { return _mipmapping; }
    };


    struct TextureMetadata
    {
        UUID_t assetID = NULL_UUID;
        UUID_t imageID = NULL_UUID;
        TextureSamplerFilterMode filterMode;
        TextureSamplerAddressMode addressMode;
        uint8_t useMipmapping = 0;
        uint8_t persistent = 0;
        char name[asset_metadata_name_size];
    };


    struct TextureImpl;
    class Texture : public Asset
    {
    private:
        TextureImpl* _pImpl = nullptr;
        const Image* _pImage = nullptr;
        // TODO: Replace above completely with the sampler ptr and test that it works!
        const TextureSampler* _pSampler = nullptr;
        uint32_t _atlasRowCount = 1;
        ImageFormat _imageFormat = ImageFormat::NONE;

    public:
        // Needed for Vulkan swapchain's color and depth textures. ...fucking dumb
        // This should ONLY create the _pImpl and set _imageFormat!
        // *This kind of Texture can't be sampled in shader!
        Texture(size_t uuidPool, ImageFormat format);
        Texture(
            size_t uuidPool,
            TextureType type,
            const TextureSampler* pSampler,
            ImageFormat format,
            uint32_t width,
            uint32_t height
        );
        Texture(
            size_t uuidPool,
            const Image* pImage,
            const TextureSampler* pSampler,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            bool persistent = false
        );
        Texture(const Texture&) = delete;
        ~Texture();

        virtual void writeToMetadataBuffer(
            std::vector<char>& targetBuffer
        ) const override;

        static Texture* create_from_metadata_buffer(
            AssetManager* pAssetManager,
            const std::vector<char>& targetBuffer,
            size_t bufferPos
        );

        static size_t get_serialized_metadata_size();

        inline const TextureImpl* getImpl() const { return _pImpl; }
        inline TextureImpl* getImpl() { return _pImpl; }
        inline const TextureSampler* getTextureSampler() const { return _pSampler; }
        inline uint32_t getAtlasRowCount() const { return _atlasRowCount; }
        inline void setAtlasRowCount(uint32_t rowCount) { _atlasRowCount = rowCount; }
        inline const Image* getImage() const { return _pImage; }
        inline ImageFormat getImageFormat() const { return _imageFormat; }
    };


    // NOTE: Should rather be member of Texture?
    void transition_image_layout(
        CommandBuffer& commandBuffer,
        Texture* pTexture,
        ImageLayout newLayout,
        PipelineStage srcStage,
        uint32_t srcAccessMask,
        PipelineStage dstStage,
        uint32_t dstAccessMask,
        uint32_t mipLevelCount = 1
    );
}
