#pragma once

#include "platypus/Common.h"
#include "Asset.h"

namespace platypus
{
    enum ImageChannelIndex
    {
        IMAGE_CHANNEL_INDEX_RED = 0,
        IMAGE_CHANNEL_INDEX_GREEN,
        IMAGE_CHANNEL_INDEX_BLUE,
        IMAGE_CHANNEL_INDEX_ALPHA
    };

    class Image : public Asset
    {
    private:
        PE_ubyte* _pData = nullptr;
        int _width = -1;
        int _height = -1;
        int _channels = -1;

    public:
        // NOTE: pData gets copied here, ownership doesn't transfer!
        Image(
            PE_ubyte* pData,
            int width,
            int height,
            int channels
        );
        ~Image();

        int getChannelValue(uint32_t x, uint32_t y, uint32_t channelIndex) const;

        static Image* load_image(const std::string& filepath);

        inline const PE_ubyte* getData() const { return _pData; }
        inline int getWidth() const { return _width; }
        inline int getHeight() const { return _height; }
        inline int getChannels() const { return _channels; }
        inline size_t getSize() const { return _width * _height * _channels; }
    };
}
