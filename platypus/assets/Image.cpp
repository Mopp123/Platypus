#include "Image.hpp"
#include "platypus/core/Debug.hpp"
#include <cstring>

// NOTE: When starting to use tinygltf we probably need to define STB_IMAGE_IMPLEMENTATION in the file
// we handle model loading so we WILL NEED TO REMOVE THIS FROM HERE!
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace platypus
{
    std::string image_format_to_string(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::NONE: return "NONE";

            case ImageFormat::R8_SRGB: return "R8_SRGB";
            case ImageFormat::R8G8B8_SRGB: return "R8G8B8_SRGB";
            case ImageFormat::R8G8B8A8_SRGB: return "R8G8B8A8_SRGB";

            case ImageFormat::B8G8R8_SRGB: return "B8G8R8_SRGB";
            case ImageFormat::B8G8R8A8_SRGB: return "B8G8R8A8_SRGB";

            case ImageFormat::R8_UNORM: return "R8_UNORM";
            case ImageFormat::R8G8B8_UNORM: return "R8G8B8_UNORM";
            case ImageFormat::R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";

            case ImageFormat::B8G8R8A8_UNORM: return "B8G8R8A8_UNORM";
            case ImageFormat::B8G8R8_UNORM: return "B8G8R8_UNORM";

            // Depth formats
            case ImageFormat::D16_UNORM: return "D16_UNORM";
            case ImageFormat::D32_SFLOAT: return "D32_SFLOAT";
            case ImageFormat::D16_UNORM_S8_UINT: return "16_UNORM_S8_UINT";
            case ImageFormat::D24_UNORM_S8_UINT: return "D24_UNORM_S8_UINT";
            case ImageFormat::D32_SFLOAT_S8_UINT: return "D32_SFLOAT_S8_UINT";

            default: return "INVALID FORMAT";
        }
    }

    size_t get_image_format_channel_count(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::NONE: return 0;

            case ImageFormat::R8_SRGB: return 1;
            case ImageFormat::R8G8B8_SRGB: return 3;
            case ImageFormat::R8G8B8A8_SRGB: return 4;

            case ImageFormat::B8G8R8_SRGB: return 3;
            case ImageFormat::B8G8R8A8_SRGB: return 4;

            case ImageFormat::R8_UNORM: return 1;
            case ImageFormat::R8G8B8_UNORM: return 3;
            case ImageFormat::R8G8B8A8_UNORM: return 4;

            case ImageFormat::B8G8R8_UNORM: return 3;
            case ImageFormat::B8G8R8A8_UNORM: return 4;

            // Depth formats
            case ImageFormat::D16_UNORM: return 1;
            case ImageFormat::D32_SFLOAT: return 1;
            case ImageFormat::D16_UNORM_S8_UINT: return 1;
            case ImageFormat::D24_UNORM_S8_UINT: return 1;
            case ImageFormat::D32_SFLOAT_S8_UINT: return 1;

            default: return 0;
        }
    }

    ImageFormat channel_count_to_image_format(size_t channelCount, bool sRGB)
    {
        if (sRGB)
        {
            switch (channelCount)
            {
                case 1: return ImageFormat::R8_SRGB;
                case 3: return ImageFormat::R8G8B8_SRGB;
                case 4: return ImageFormat::R8G8B8A8_SRGB;
            }
        }

        switch (channelCount)
        {
            case 1: return ImageFormat::R8_UNORM;
            case 3: return ImageFormat::R8G8B8_UNORM;
            case 4: return ImageFormat::R8G8B8A8_UNORM;
        }

        Debug::log(
            "@channel_count_to_image_format "
            "Unsupported channel count: " + std::to_string(channelCount),
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return ImageFormat::R8_SRGB;
    }

    bool is_image_format_valid(ImageFormat format, int channels)
    {
        if ((format == ImageFormat::R8_SRGB || format == ImageFormat::R8_UNORM || format == ImageFormat::D32_SFLOAT) &&
            channels == 1)
        {
            return true;
        }
        else if ((format == ImageFormat::R8G8B8_SRGB || format == ImageFormat::R8G8B8_UNORM || format == ImageFormat::B8G8R8_SRGB || format == ImageFormat::B8G8R8_UNORM) &&
                channels == 3)
        {
            return true;
        }
        else if ((format == ImageFormat::R8G8B8A8_SRGB || format == ImageFormat::R8G8B8A8_UNORM || format == ImageFormat::B8G8R8A8_SRGB || format == ImageFormat::B8G8R8A8_SRGB) &&
                channels == 4)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool is_color_format(ImageFormat format)
    {
        ImageFormat firstDepthFormat = ImageFormat::D16_UNORM;
        ImageFormat lastDepthFormat = ImageFormat::D32_SFLOAT_S8_UINT;
        return format < firstDepthFormat || format > lastDepthFormat;
    }

    ImageFormat srgb_format_to_unorm(ImageFormat srgb)
    {
        switch (srgb)
        {
            case ImageFormat::R8_SRGB: return ImageFormat::R8_UNORM;
            case ImageFormat::R8G8B8_SRGB: return ImageFormat::R8G8B8_UNORM;
            case ImageFormat::R8G8B8A8_SRGB: return ImageFormat::R8G8B8A8_UNORM;

            case ImageFormat::B8G8R8A8_SRGB: return ImageFormat::B8G8R8A8_UNORM;
            case ImageFormat::B8G8R8_SRGB: return ImageFormat::B8G8R8_UNORM;

            default: {
                Debug::log(
                    "@srgb_format_to_unorm "
                    "Invalid SRGB image format",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
        return ImageFormat::NONE;
    }


    Image::Image(
        PE_ubyte* pData,
        int width,
        int height,
        int channels,
        ImageFormat format
    ) :
        Asset(AssetType::ASSET_TYPE_IMAGE),
        _width(width),
        _height(height),
        _channels(channels),
        _format(format)
    {
        if (pData)
        {
            const size_t size = getSize();
            _pData = new PE_ubyte[size];
            memcpy(_pData, pData, size);
        }
    }

    Image::~Image()
    {
        if (_pData)
            delete[] _pData;
    }

    Image* Image::load_image(const std::string& filepath, ImageFormat format)
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        // TODO: On OpenGL side we need to flip?
        bool flipVertically = false;
        stbi_set_flip_vertically_on_load(flipVertically);
        unsigned char* pStbImageData = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

        if (!pStbImageData)
        {
            stbi_image_free(pStbImageData);
            return nullptr;
        }
        Image* pImage = new Image(pStbImageData, width, height, channels, format);
        stbi_image_free(pStbImageData);
        return pImage;
    }

    int Image::getColorChannelValue(
        uint32_t x,
        uint32_t y,
        uint32_t channelIndex
    ) const
    {
        if (!_pData)
        {
            Debug::log(
                "@Image::getColorChannelValue "
                "Image data was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return 0;
        }
        if (channelIndex >= _channels)
        {
            Debug::log(
                "@Image::getColorChannelValue "
                "Invalid channel index(" + std::to_string(channelIndex) + ") "
                "this image has " + std::to_string(_channels) + " channels.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return 0;
        }
        if (x >= _width || y >= _height)
        {
            Debug::log(
                "@Image::getColorChannelValue "
                "Image coordinates(" + std::to_string(x) + ", " + std::to_string(y) + ") "
                "out of bounds of the image! "
                "Image dimensions: " + std::to_string(_width) + "x" + std::to_string(_height),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return 0;
        }
        return _pData[(x + y * _width) * _channels + channelIndex];
    }
}
