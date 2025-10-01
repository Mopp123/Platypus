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

    enum class TextureUsage
    {
        NONE,
        TEXTURE2D,
        SWAPCHAIN_FRAMEBUFFER_COLOR,
        SWAPCHAIN_FRAMEBUFFER_DEPTH,
        FRAMEBUFFER_COLOR,
        FRAMEBUFFER_DEPTH
    };

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
        const Image* _pImage = nullptr;
        std::shared_ptr<const TextureSamplerImpl> _pSamplerImpl = nullptr;
        uint32_t _atlasRowCount = 1;

    public:
        // Needed for Vulkan swapchain's color and depth textures. ...fucking dumb
        // This should ONLY create the _pImpl!
        Texture(bool empty);
        Texture(
            TextureUsage usage,
            const TextureSampler& sampler,
            ImageFormat format,
            uint32_t width,
            uint32_t height
        );
        Texture(
            const Image* pImage,
            const TextureSampler& sampler,
            uint32_t atlasRowCount = 1
        );
        Texture(const Texture&) = delete;
        ~Texture();

        inline const TextureImpl* getImpl() const { return _pImpl; }
        inline TextureImpl* getImpl() { return _pImpl; }
        inline const std::shared_ptr<const TextureSamplerImpl>& getSamplerImpl() const { return _pSamplerImpl; }
        inline uint32_t getAtlasRowCount() const { return _atlasRowCount; }
        inline void setAtlasRowCount(uint32_t rowCount) { _atlasRowCount = rowCount; }
        inline const Image* getImage() const { return _pImage; }
    };
}
