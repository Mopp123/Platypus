#pragma once

#include "platypus/utils/UUID.hpp"
#include <string>
#include <vector>


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


    constexpr size_t asset_metadata_name_size = 32; // NOTE: This is probably too short!
    constexpr size_t asset_metadata_filepath_size = 64;
    constexpr size_t asset_metadata_model_max_meshes = 8;
    constexpr size_t serialized_assets_header_size = sizeof(uint32_t) * 4;


    class AssetManager;
    class Asset
    {
    protected:
        UUID_t _id = NULL_UUID;
        AssetType _type = AssetType::ASSET_TYPE_NONE;
        std::string _name;
        bool _persistent = false;
        bool _serializable = true;

    public:
        Asset() = delete;
        Asset(const Asset&) = delete;
        Asset(
            AssetType type,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            bool persistent = false
        );
        virtual ~Asset();

        virtual void writeToMetadataBuffer(
            std::vector<char>& targetBuffer
        ) const { }

        // TODO: Maybe this should be member var of Asset..?
        inline bool isPersistent() const { return _persistent; }
        inline void setPersistent(bool arg) { _persistent = arg; }

        inline UUID_t getID() const { return _id; }
        inline AssetType getType() const { return _type; }
        inline const std::string& getName() const { return _name; }
        inline void setSerializable(bool arg) { _serializable = arg; }
        inline bool isSerializable() const { return _serializable; }
    };
}
