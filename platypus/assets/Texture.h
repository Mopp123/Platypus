#pragma once

#include "Asset.h"
#include "platypus/assets/Image.h"
#include "platypus/graphics/CommandBuffer.h"
#include <memory>


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


    // NOTE: Atm TextureSampler is supposed to be passed around as const ref!
    // That's why pImpl is shared_ptr in this case
    struct TextureSamplerImpl;
    class TextureSampler
    {
    private:
        std::shared_ptr<TextureSamplerImpl> _pImpl = nullptr;

    public:
        TextureSampler(
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            uint32_t mipLevelCount,
            uint32_t anisotropicFiltering
        );
        ~TextureSampler();
        TextureSampler(const TextureSampler& other);

        inline const std::shared_ptr<TextureSamplerImpl>& getImpl() const { return _pImpl; }
    };


    struct TextureImpl;
    class Texture : public Asset
    {
    private:
        TextureImpl* _pImpl = nullptr;
        std::shared_ptr<const TextureSamplerImpl> _pSamplerImpl = nullptr;

    public:
        Texture(
            const CommandPool& commandPool,
            const Image* pImage,
            const TextureSampler& pSampler
        );
        Texture(const Texture&) = delete;
        ~Texture();

        inline const std::shared_ptr<const TextureSamplerImpl>& getSamplerImpl() const { return _pSamplerImpl; }
        inline const TextureImpl* getImpl() const { return _pImpl; }
    };
}
