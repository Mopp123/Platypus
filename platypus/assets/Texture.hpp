#pragma once

#include "Asset.hpp"
#include "Image.hpp"
#include "platypus/graphics/CommandBuffer.hpp"
#include "platypus/graphics/Pipeline.hpp"
#include <memory>


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
        Texture(ImageFormat format);
        Texture(
            TextureType type,
            const TextureSampler* pSampler,
            ImageFormat format,
            uint32_t width,
            uint32_t height
        );
        Texture(
            const Image* pImage,
            const TextureSampler* pSampler,
            uint32_t atlasRowCount = 1
        );
        Texture(const Texture&) = delete;
        ~Texture();

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
