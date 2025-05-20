#include "platypus/assets/Texture.h"
#include "WebTexture.h"
#include "platypus/graphics/platform/web/WebContext.h"
#include "platypus/core/Debug.h"


#include <GL/glew.h>


namespace platypus
{
    struct TextureSamplerImpl
    {
    };

    TextureSampler::TextureSampler(
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        uint32_t mipLevelCount,
        uint32_t anisotropicFiltering
    ) :
        _filterMode(filterMode),
        _addressMode(addressMode),
        _mipLevelCount(mipLevelCount)
    {
    }

    TextureSampler::~TextureSampler()
    {
    }

    TextureSampler::TextureSampler(const TextureSampler& other) :
        _filterMode(other._filterMode),
        _addressMode(other._addressMode),
        _mipLevelCount(other._mipLevelCount)
    {
    }


    Texture::Texture(
        const CommandPool& commandPool,
        const Image* pImage,
        ImageFormat targetFormat,
        const TextureSampler& sampler,
        uint32_t atlasRowCount
    ) :
        Asset(AssetType::ASSET_TYPE_TEXTURE),
        _atlasRowCount(atlasRowCount)
    {
        if (!is_image_format_valid(targetFormat, pImage->getChannels()))
        {
            Debug::log(
                "@Texture::Texture "
                "Invalid target format: " + image_format_to_string(targetFormat) + " "
                "for image with " + std::to_string(pImage->getChannels()) + " channels",
                Debug::MessageType::PLATYPUS_ERROR
            );
        }

        if (!pImage)
        {
            Debug::log(
                "@Texture::Texture "
                "Attempted to create texture without providing image data. "
                "This is currently illegal for web platform!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return;
        }
        if (!pImage->getData())
        {
            Debug::log(
                "@Texture::Texture "
                "Attempted to create texture with invalid image data. "
                "(image's data was nullptr)",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return;
        }

        GLint glFormat = 0;
        const int channels = pImage->getChannels();
        const int width = pImage->getWidth();
        const int height = pImage->getHeight();

        switch (channels)
        {
            // NOTE: webgl seems to have GL_ALPHA but on modern opengl it doesn't exist
            // -> "desktop opengl" needs to convert this to single red channel
            case 1: glFormat = GL_ALPHA; break;
            case 3: glFormat = GL_RGB; break;
            case 4: glFormat = GL_RGBA; break;
            default:
                Debug::log(
                    "@Texture::Texture "
                    "Invalid color channel count: " + std::to_string(channels) + " "
                    "Currently 4 channels are required (for some reason doesn't work on web if no 4 channels)",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                break;
        }

        uint32_t id = 0;
        GL_FUNC(glGenTextures(1, &id));
        GL_FUNC(glBindTexture(GL_TEXTURE_2D, id));
        GL_FUNC(glTexImage2D(
            GL_TEXTURE_2D,
            0,
            glFormat,
            width,
            height,
            0,
            glFormat,
            GL_UNSIGNED_BYTE,
            (const void*)pImage->getData()
        ));

        // Address mode
        switch (sampler.getAddressMode())
        {
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT :
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
                break;
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
                // *It appears that webgl 1.0 doesnt have "GL_CLAMP_TO_BORDER"?
                Debug::log(
                    "@Texture::Texture "
                    "Using sampler address mode SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER "
                    "which is not supported(at least in webgl 1.0?) switching to "
                    "SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE",
                    Debug::MessageType::PLATYPUS_WARNING
                );
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                break;
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                break;
            default:
                Debug::log(
                    "Invalid sampler address mode while trying to create OpenglTexture",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                break;
        }

        if (sampler.getMipLevelCount() > 1)
        {
            if(sampler.getFilterMode() == TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR)
            {
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            }
            else if (sampler.getFilterMode() == TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR)
            {
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            }

            GL_FUNC(glGenerateMipmap(GL_TEXTURE_2D));
        }
        else
        {
            if (sampler.getFilterMode() == TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR)
            {
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            }
            else if (sampler.getFilterMode() == TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR)
            {
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
                GL_FUNC(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            }
        }
        // NOTE: Not sure why not previously unbinding the texture here??
        GL_FUNC(glBindTexture(GL_TEXTURE_2D, 0));

        _pImpl = new TextureImpl;
        _pImpl->id = id;

        Debug::log("___TEST___ texture created successfully");
    }

    Texture::~Texture()
    {
        if (_pImpl)
        {
            glDeleteTextures(1, &_pImpl->id);
            delete _pImpl;
        }
    }
}
