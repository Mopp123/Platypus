#pragma once

#include "platypus/utils/ID.h"
#include <string>


namespace platypus
{
    enum AssetType
    {
        ASSET_TYPE_NONE = 0,
        ASSET_TYPE_MESH = 1
    };

    std::string asset_type_to_string(AssetType type);

    class Asset
    {
    private:
        ID_t _id = NULL_ID;
        AssetType _type = AssetType::ASSET_TYPE_NONE;

    public:
        Asset(AssetType type);
        virtual ~Asset();

        inline ID_t getID() const { return _id; }
        inline AssetType getType() const { return _type; }
    };
}
