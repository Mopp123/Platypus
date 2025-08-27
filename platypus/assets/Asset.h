#pragma once

#include "platypus/utils/ID.h"
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
