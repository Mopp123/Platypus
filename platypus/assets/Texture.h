#pragma once

#include "Asset.h"
#include "platypus/assets/Image.h"
#include "platypus/graphics/CommandBuffer.h"


namespace platypus
{
    enum class TextureSamplerFilterMode
    {
        SAMPLER_FILTER_MODE_LINEAR,
        SAMPLER_FILTER_MODE_NEAR
    };

    enum class TextureSamplerAddressMode
    {
        SAMPLER_ADDRESS_MODE_REPEAT,
        SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
    };


    struct TextureSamplerImpl;
    class TextureSampler
    {
    private:
        TextureSamplerImpl* _pImpl = nullptr;

    public:
        TextureSampler(
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            uint32_t mipLevelCount,
            uint32_t anisotropicFiltering
        );
        ~TextureSampler();

        inline const TextureSamplerImpl* getImpl() const { return _pImpl; }
    };


    struct TextureImpl;
    class Texture : public Asset
    {
    private:
        TextureImpl* _pImpl = nullptr;
        const TextureSampler* _pSampler = nullptr;

    public:
        Texture(const CommandPool& commandPool, const Image* pImage, const TextureSampler* pSampler);
        Texture(const Texture&) = delete;
        ~Texture();

        inline const TextureSampler * const getSampler() const { return _pSampler; }
        inline const TextureImpl* getImpl() const { return _pImpl; }
    };
}
