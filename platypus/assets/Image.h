#pragma once

#include "platypus/Common.h"
#include "Asset.h"

#define PE_IMAGE_MAX_CHANNELS 4


namespace platypus
{
    enum class ImageFormat
    {
        NONE,

        // Color formats
        R8_SRGB,
        R8G8B8_SRGB,
        R8G8B8A8_SRGB,

        B8G8R8A8_SRGB,
        B8G8R8_SRGB,

        R8_UNORM,
        R8G8B8_UNORM,
        R8G8B8A8_UNORM,

        B8G8R8A8_UNORM,
        B8G8R8_UNORM,

        // Depth formats
        D16_UNORM,
        D32_SFLOAT,
        D16_UNORM_S8_UINT,
        D24_UNORM_S8_UINT,
        D32_SFLOAT_S8_UINT
    };

    enum class ImageLayout
    {
        UNDEFINED,
        TRANSFER_DST_OPTIMAL,
        SHADER_READ_ONLY_OPTIMAL,
        COLOR_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        PRESENT_SRC_KHR
    };

    enum ImageChannelIndex
    {
        IMAGE_CHANNEL_INDEX_RED = 0,
        IMAGE_CHANNEL_INDEX_GREEN,
        IMAGE_CHANNEL_INDEX_BLUE,
        IMAGE_CHANNEL_INDEX_ALPHA
    };

    std::string image_format_to_string(ImageFormat format);
    size_t get_image_format_channel_count(ImageFormat format);
    ImageFormat channel_count_to_image_format(size_t channelCount, bool sRGB);
    bool is_image_format_valid(ImageFormat format, int channels);
    bool is_color_format(ImageFormat format);
    ImageFormat srgb_format_to_unorm(ImageFormat srgb);

    struct ImageImpl;
    class Image : public Asset
    {
    private:
        ImageImpl* _pImpl = nullptr;
        PE_ubyte* _pData = nullptr;
        int _width = -1;
        int _height = -1;
        int _channels = -1;
        ImageFormat _format;

    public:
        // NOTE: pData gets copied here, ownership doesn't transfer!
        Image(
            PE_ubyte* pData,
            int width,
            int height,
            int channels,
            ImageFormat format
        );
        ~Image();

        static Image* load_image(const std::string& filepath, ImageFormat format);
        int getColorChannelValue(
            uint32_t x,
            uint32_t y,
            uint32_t channelIndex
        ) const;

        inline ImageImpl* getImpl() { return _pImpl; }
        inline const PE_ubyte* getData() const { return _pData; }
        inline int getWidth() const { return _width; }
        inline int getHeight() const { return _height; }
        inline int getChannels() const { return _channels; }
        inline size_t getSize() const { return _width * _height * _channels; }
        inline ImageFormat getFormat() const { return _format; }
    };

}
