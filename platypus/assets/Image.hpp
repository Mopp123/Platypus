#pragma once

#include "platypus/Common.h"
#include "Asset.hpp"
#include <vector>

#define PE_IMAGE_MAX_CHANNELS 4


namespace platypus
{
    enum class ImageFormat : uint32_t
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
    ImageFormat string_to_image_format(const std::string& str);
    size_t get_image_format_channel_count(ImageFormat format);
    ImageFormat channel_count_to_image_format(size_t channelCount, bool sRGB);
    bool is_image_format_valid(ImageFormat format, int channels);
    bool is_color_format(ImageFormat format);
    ImageFormat srgb_format_to_unorm(ImageFormat srgb);


    struct ImageMetadata
    {
        UUID_t assetID = NULL_UUID;
        ImageFormat format;
        uint8_t persistent = 0;
        char name[asset_metadata_name_size];
        char filepath[asset_metadata_filepath_size];
    };


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
        std::string _filepath;

    public:
        // NOTE: pData gets copied here, ownership doesn't transfer!
        Image(
            size_t uuidPool,
            PE_ubyte* pData,
            int width,
            int height,
            int channels,
            ImageFormat format,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            bool persistent = false
        );
        ~Image();

        int getColorChannelValue(
            uint32_t x,
            uint32_t y,
            uint32_t channelIndex
        ) const;

        bool reload(const std::string& newFilepath, ImageFormat format);

        static Image* load_image(
            size_t uuidPool,
            const std::string& filepath,
            ImageFormat format,
            const std::string& name = "",
            UUID_t id = NULL_UUID
        );

        // NOTE: This allocates ppPixels on heap which you'll need to free at some point!
        static bool read_image_pixels(
            const std::string& filepath,
            int* pOutWidth,
            int* pOutHeight,
            int* pOutChannels,
            PE_ubyte** ppPixels
        );

        virtual void writeToMetadataBuffer(
            std::vector<char>& targetBuffer
        ) const override;

        static Image* create_from_metadata_buffer(
            AssetManager* pAssetManager,
            const std::vector<char>& targetBuffer,
            size_t bufferPos
        );

        static size_t get_serialized_metadata_size();

        inline const std::string& getFilepath() const { return _filepath; }
        inline void setFilepath(const std::string& filepath) { _filepath = filepath; }

        inline ImageImpl* getImpl() { return _pImpl; }
        inline const PE_ubyte* getData() const { return _pData; }
        inline int getWidth() const { return _width; }
        inline int getHeight() const { return _height; }
        inline int getChannels() const { return _channels; }
        inline size_t getSize() const { return _width * _height * _channels; }
        inline ImageFormat getFormat() const { return _format; }
    };
}
