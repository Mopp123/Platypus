#pragma once

#include "platypus/assets/Image.h"
#include "CommandBuffer.h"


namespace platypus
{
    struct TextureImpl;
    class Texture
    {
    private:
        TextureImpl* _pImpl = nullptr;

    public:
        Texture(const CommandPool& commandPool, const Image* pImage);
        Texture(const Texture&) = delete;
        ~Texture();

        inline TextureImpl* getImpl() { return _pImpl; }
    };
}
