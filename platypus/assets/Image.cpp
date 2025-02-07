#include "Image.h"
#include "platypus/core/Debug.h"

// NOTE: When starting to use tinygltf we probably need to define STB_IMAGE_IMPLEMENTATION in the file
// we handle model loading so we WILL NEED TO REMOVE THIS FROM HERE!
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace platypus
{
    Image::Image(
        PE_ubyte* pData,
        int width,
        int height,
        int channels
    ) :
        Asset(AssetType::ASSET_TYPE_IMAGE),
        _width(width),
        _height(height),
        _channels(channels)
    {
        const size_t size = getSize();
        _pData = new PE_ubyte[size];
        memcpy(_pData, pData, size);
    }

    Image::~Image()
    {
        if (_pData)
            delete[] _pData;
    }

    Image* Image::load_image(const std::string& filepath)
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

        Image* pImage = new Image(pStbImageData, width, height, channels);
        stbi_image_free(pStbImageData);
        return pImage;
    }
}
