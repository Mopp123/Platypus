#pragma once

#include "platypus/utils/ID.hpp"
#include <string>


namespace platypus
{
    enum class AssetType
    {
        ASSET_TYPE_NONE,
        ASSET_TYPE_MESH,
        ASSET_TYPE_MODEL,
        ASSET_TYPE_IMAGE,
        ASSET_TYPE_TEXTURE,
        ASSET_TYPE_MATERIAL,
        ASSET_TYPE_FONT,
        ASSET_TYPE_SKELETAL_ANIMATION_DATA
    };

    std::string asset_type_to_string(AssetType type);


    // NOTE: Atm these follows Vulkan's VkAccessFlagBits
    enum MemoryAccessFlagBits
    {
        MEMORY_ACCESS_SHADER_READ_BIT = 0x00000020,

        MEMORY_ACCESS_COLOR_ATTACHMENT_READ_BIT = 0x00000080,
        MEMORY_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,

        MEMORY_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
        MEMORY_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,

        MEMORY_ACCESS_TRANSFER_WRITE_BIT = 0x00001000
    };


    class Asset
    {
    private:
        ID_t _id = NULL_ID;
        AssetType _type = AssetType::ASSET_TYPE_NONE;

    public:
        Asset() = delete;
        Asset(const Asset&) = delete;
        Asset(AssetType type);
        virtual ~Asset();

        inline ID_t getID() const { return _id; }
        inline AssetType getType() const { return _type; }
    };
}
