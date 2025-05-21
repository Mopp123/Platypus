#pragma once

#include "Asset.h"
#include "platypus/assets/Image.h"
#include "platypus/graphics/CommandBuffer.h"
#include <string>
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

    enum class ImageFormat
    {
        R8_SRGB,
        R8G8B8_SRGB,
        R8G8B8A8_SRGB,

        R8_UNORM,
        R8G8B8_UNORM,
        R8G8B8A8_UNORM
    };

    std::string image_format_to_string(ImageFormat format);

    bool is_image_format_valid(ImageFormat imageFormat, int imageColorChannels);


    // NOTE: Atm TextureSampler is supposed to be passed around as const ref!
    // That's why pImpl is shared_ptr in this case
    struct TextureSamplerImpl;
    class TextureSampler
    {
    private:
        std::shared_ptr<TextureSamplerImpl> _pImpl = nullptr;
        TextureSamplerFilterMode _filterMode;
        TextureSamplerAddressMode _addressMode;
        bool _mipmapping = false;

    public:
        TextureSampler(
            TextureSamplerFilterMode filterMode,
            TextureSamplerAddressMode addressMode,
            bool mipmapping,
            uint32_t anisotropicFiltering
        );
        ~TextureSampler();
        TextureSampler(const TextureSampler& other);

        inline TextureSamplerFilterMode getFilterMode() const { return _filterMode; }
        inline TextureSamplerAddressMode getAddressMode() const { return _addressMode; }
        inline const std::shared_ptr<TextureSamplerImpl>& getImpl() const { return _pImpl; }
        inline bool isMipmapped() const { return _mipmapping; }
    };


    struct TextureImpl;
    class Texture : public Asset
    {
    private:
        TextureImpl* _pImpl = nullptr;
        std::shared_ptr<const TextureSamplerImpl> _pSamplerImpl = nullptr;
        uint32_t _atlasRowCount = 1;

    public:
        Texture(
            const CommandPool& commandPool,
            const Image* pImage,
            ImageFormat targetFormat,
            const TextureSampler& sampler,
            uint32_t atlasRowCount = 1
        );
        Texture(const Texture&) = delete;
        ~Texture();

        inline const TextureImpl* getImpl() const { return _pImpl; }
        inline const std::shared_ptr<const TextureSamplerImpl>& getSamplerImpl() const { return _pSamplerImpl; }
        inline uint32_t getAtlasRowCount() const { return _atlasRowCount; }
        inline void setAtlasRowCount(uint32_t rowCount) { _atlasRowCount = rowCount; }
    };
}
