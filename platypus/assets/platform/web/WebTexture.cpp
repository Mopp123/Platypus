#include "platypus/assets/Texture.h"
#include "WebTexture.h"
#include "platypus/graphics/platform/web/WebContext.hpp"
#include "platypus/core/Debug.h"

#include <GL/glew.h>
#include <GL/glext.h>


namespace platypus
{
    GLenum to_gl_internal_format(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::R8_SRGB: return GL_RED;
            case ImageFormat::R8G8B8_SRGB: return GL_SRGB8;//GL_RGB;
            case ImageFormat::R8G8B8A8_SRGB: return GL_SRGB8_ALPHA8;//GL_RGBA;

            // NOTE: OpenGL ES 3 doesn't seem to have BGR, etc formats so not sure if
            // this eventually fucks up something...
            case ImageFormat::B8G8R8A8_SRGB: return GL_RGBA;
            case ImageFormat::B8G8R8_SRGB: return GL_RGB;

            case ImageFormat::R8_UNORM: return GL_R8;
            case ImageFormat::R8G8B8_UNORM: return GL_RGB;
            case ImageFormat::R8G8B8A8_UNORM: return GL_RGBA;

            // NOTE: OpenGL ES 3 doesn't seem to have BGR, etc formats so not sure if
            // this eventually fucks up something...
            case ImageFormat::B8G8R8A8_UNORM: return GL_RGBA;
            case ImageFormat::B8G8R8_UNORM: return GL_RGB;

            // Depth formats
            // NOTE: Only D32_SFLOAT has been tested from these!
            case ImageFormat::D16_UNORM: return GL_DEPTH_COMPONENT16;
            case ImageFormat::D32_SFLOAT: return GL_FLOAT;
            case ImageFormat::D16_UNORM_S8_UINT: return GL_DEPTH_COMPONENT16;
            case ImageFormat::D24_UNORM_S8_UINT: return GL_DEPTH_COMPONENT24;
            case ImageFormat::D32_SFLOAT_S8_UINT: return GL_FLOAT;
        }
        PLATYPUS_ASSERT(false);
        return GL_RED;
    }

    GLenum to_gl_format(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::R8_SRGB: return GL_RED;
            case ImageFormat::R8G8B8_SRGB: return GL_RGB;
            case ImageFormat::R8G8B8A8_SRGB: return GL_RGBA;

            // NOTE: OpenGL ES 3 doesn't seem to have BGR, etc formats so not sure if
            // this eventually fucks up something...
            case ImageFormat::B8G8R8A8_SRGB: return GL_RGBA;
            case ImageFormat::B8G8R8_SRGB: return GL_RGB;

            case ImageFormat::R8_UNORM: return GL_RED;
            case ImageFormat::R8G8B8_UNORM: return GL_RGB;
            case ImageFormat::R8G8B8A8_UNORM: return GL_RGBA;

            // NOTE: OpenGL ES 3 doesn't seem to have BGR, etc formats so not sure if
            // this eventually fucks up something...
            case ImageFormat::B8G8R8A8_UNORM: return GL_RGBA;
            case ImageFormat::B8G8R8_UNORM: return GL_RGB;

            // Depth formats
            // NOTE: Only D32_SFLOAT has been tested from these!
            case ImageFormat::D16_UNORM: return GL_DEPTH_COMPONENT16;
            case ImageFormat::D32_SFLOAT: return GL_FLOAT;
            case ImageFormat::D16_UNORM_S8_UINT: return GL_DEPTH_COMPONENT16;
            case ImageFormat::D24_UNORM_S8_UINT: return GL_DEPTH_COMPONENT24;
            case ImageFormat::D32_SFLOAT_S8_UINT: return GL_FLOAT;
        }
        PLATYPUS_ASSERT(false);
        return GL_RED;
    }

    struct TextureSamplerImpl
    {
    };

    TextureSampler::TextureSampler(
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        bool mipmapping,
        uint32_t anisotropicFiltering
    ) :
        _filterMode(filterMode),
        _addressMode(addressMode),
        _mipmapping(mipmapping)
    {
    }

    TextureSampler::~TextureSampler()
    {
    }

    TextureSampler::TextureSampler(const TextureSampler& other) :
        _filterMode(other._filterMode),
        _addressMode(other._addressMode),
        _mipmapping(other._mipmapping)
    {
    }


    Texture::Texture(bool empty) :
        Asset(AssetType::ASSET_TYPE_TEXTURE)
    {
        _pImpl = new TextureImpl;
    }

    Texture::Texture(
        TextureType type,
        const TextureSampler& sampler,
        ImageFormat format,
        uint32_t width,
        uint32_t height
    ) :
        Asset(AssetType::ASSET_TYPE_TEXTURE)
    {
        GLint glInternalFormat = 0;
        GLenum glFormat = 0;
        GLenum glType = 0;

        if (type == TextureType::COLOR_TEXTURE)
        {
            glInternalFormat = to_gl_internal_format(format);
            glFormat = to_gl_format(format);
            glType = GL_UNSIGNED_BYTE;
        }
        else if (type == TextureType::DEPTH_TEXTURE)
        {
            glInternalFormat = GL_DEPTH_COMPONENT32F;
            glFormat = GL_DEPTH_COMPONENT;
            glType = GL_FLOAT;
        }

        uint32_t id = 0;
        GL_FUNC(glGenTextures(1, &id));
        GL_FUNC(glBindTexture(GL_TEXTURE_2D, id));
        GL_FUNC(glTexImage2D(
            GL_TEXTURE_2D,
            0,
            glInternalFormat,
            width,
            height,
            0,
            glFormat,
            glType,
            0
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

        // NOTE: Not sure if should allowing mipmapping with framebuffer attachment textures...
        if (sampler.isMipmapped())
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
    }

    Texture::Texture(
        const Image* pImage,
        const TextureSampler& sampler,
        uint32_t atlasRowCount
    ) :
        Asset(AssetType::ASSET_TYPE_TEXTURE),
        _pImage(pImage),
        _atlasRowCount(atlasRowCount)
    {
        ImageFormat imageFormat = _pImage->getFormat();
        if (!is_image_format_valid(imageFormat, pImage->getChannels()))
        {
            Debug::log(
                "@Texture::Texture "
                "Invalid target format: " + image_format_to_string(imageFormat) + " "
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

        // NOTE: webgl seems to have GL_ALPHA but on modern opengl it doesn't exist
        // -> "desktop opengl" needs to convert this to single red channel
        // UPDATE: This was problem at some point, but might have been fixed by swithcing to
        // WebGL2 (GLES3)?
        GLint glFormat = to_gl_format(imageFormat);
        GLint glInternalFormat = to_gl_internal_format(imageFormat);

        const int width = pImage->getWidth();
        const int height = pImage->getHeight();

        uint32_t id = 0;
        GL_FUNC(glGenTextures(1, &id));
        GL_FUNC(glBindTexture(GL_TEXTURE_2D, id));
        GL_FUNC(glTexImage2D(
            GL_TEXTURE_2D,
            0,
            glInternalFormat,
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

        if (sampler.isMipmapped())
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
    }

    Texture::~Texture()
    {
        if (_pImpl)
        {
            glDeleteTextures(1, &_pImpl->id);
            delete _pImpl;
        }
    }

    void transition_image_layout(
        CommandBuffer& commandBuffer,
        Texture* pTexture,
        ImageLayout newLayout,
        PipelineStage srcStage,
        uint32_t srcAccessMask,
        PipelineStage dstStage,
        uint32_t dstAccessMask,
        uint32_t mipLevelCount
    )
    {
    }
}
