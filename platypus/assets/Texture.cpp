#include "Texture.h"

namespace platypus
{
    bool is_image_format_valid(ImageFormat imageFormat, int imageColorChannels)
    {
        if ((imageFormat == ImageFormat::R8_SRGB || imageFormat == ImageFormat::R8_UNORM) && imageColorChannels == 1)
            return true;
        else if ((imageFormat == ImageFormat::R8G8B8_SRGB || imageFormat == ImageFormat::R8G8B8_UNORM) && imageColorChannels == 3)
            return true;
        else if ((imageFormat == ImageFormat::R8G8B8A8_SRGB || imageFormat == ImageFormat::R8G8B8A8_UNORM) && imageColorChannels == 4)
            return true;
        else
            return false;
    }

    std::string image_format_to_string(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::R8_SRGB: return "R8_SRGB";
            case ImageFormat::R8G8B8_SRGB: return "R8G8B8_SRGB";
            case ImageFormat::R8G8B8A8_SRGB: return "R8G8B8A8_SRGB";

            case ImageFormat::R8_UNORM: return "R8_UNORM";
            case ImageFormat::R8G8B8_UNORM: return "R8G8B8_UNORM";
            case ImageFormat::R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";

            default: return "INVALID FORMAT";
        }
    }
}
